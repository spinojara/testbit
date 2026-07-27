#include "clop.h"

#include <unordered_map>
#include <memory>
#include <iostream>
#include <map>
#include <utility>
#include <chrono>
#include <random>

#include "CParameterCollection.h"
#include "CResults.h"
#include "CMESampleMean.h"
#include "CSPWeight.h"
#include "CRegression.h"
#include "CPFQuadratic.h"
#include "CLinearParameter.h"
#include "COutcome.h"
#include "CObserver.h"

extern "C" {
#include "cjson/cJSON.h"
#include "sql.h"
}

class CWeightUpdater;

class CExperiment {
private:
	pthread_mutex_t lock_;
	std::vector<std::unique_ptr<CParameter>> params;
	CParameterCollection paramcol;
public:
	CWeightUpdater *wu;
	CResults results;
	CPFQuadratic pf;
	CRegression reg;
	CMESampleMean me;
	CSPWeight sp;
	CExperiment(std::vector<std::unique_ptr<CParameter>> params_, CParameterCollection paramcol_, unsigned seed)
		: params(std::move(params_)),
		paramcol(std::move(paramcol_)),
		results(paramcol.GetSize()),
		pf(paramcol.GetSize()),
		reg(results, pf),
		me(reg), sp(reg)
	{
		pthread_mutex_init(&lock_, NULL);
		sp.Seed(seed);
		wu = nullptr;
	}

	~CExperiment(void);

	void lock(void) {
		pthread_mutex_lock(&lock_);
	}

	void unlock(void) {
		pthread_mutex_unlock(&lock_);
	}

	std::vector<double> json_to_point(cJSON *spsa) {
		std::vector<double> v;
		for (int i = 0; i < paramcol.GetSize(); i++) {
			const CParameter &p = paramcol.GetParam(i);
			cJSON *item = cJSON_GetObjectItemCaseSensitive(spsa, p.GetName().c_str());
			v.push_back(p.TransformToQLR(item->valuedouble));
		}

		return v;
	}

	cJSON *point_to_json(const double *v) {
		cJSON *spsa = cJSON_CreateObject();

		for (int i = 0; i < paramcol.GetSize(); i++) {
			const CParameter &p = paramcol.GetParam(i);
			std::cout << p.TransformFromQLR(v[i]) << std::endl;
			cJSON_AddNumberToObject(spsa, p.GetName().c_str(), p.TransformFromQLR(v[i]));
		}


		return spsa;
	}
};

class CWeightUpdater : CObserver {
	CExperiment &cexp;
	int id;
public:
	CWeightUpdater(CExperiment &cexp_, int id_) : CObserver(cexp_.results), cexp(cexp_), id(id_) {}

	void OnOutcome(int i) override {
		printf("Updating all database weights!\n");
		pthread_mutex_lock(&db_lock);
		sqlite3_exec(db, "BEGIN", NULL, NULL, NULL);

		sqlite3_stmt *stmt;
		sqlite3_prepare_v2(db,
			"SELECT id, spsa\n"
			"FROM games\n"
			"WHERE testid = ?\n"
				"AND donetime IS NOT NULL\n"
				"AND spsa IS NOT NULL;",
			-1, &stmt, NULL);
		sqlite3_bind_int(stmt, 1, id);

		sqlite3_stmt *stmt2;
		sqlite3_prepare_v2(db,
			"UPDATE games\n"
			"SET weight = ?\n"
			"WHERE id = ?;",
			-1, &stmt2, NULL);

		while (sqlite3_step(stmt) == SQLITE_ROW) {
			int id = sqlite3_column_int(stmt, 0);
			cJSON *spsa = cJSON_Parse((const char *)sqlite3_column_text(stmt, 1));
			std::vector<double> v = cexp.json_to_point(spsa);
			cJSON_Delete(spsa);
			double weight = cexp.reg.GetWeight(&v[0]);
			sqlite3_bind_double(stmt2, 1, weight);
			sqlite3_bind_int(stmt2, 2, id);

			sqlite3_step(stmt2);

			sqlite3_reset(stmt2);
		}

		sqlite3_finalize(stmt2);
		sqlite3_finalize(stmt);

		sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
		pthread_mutex_unlock(&db_lock);
		printf("Done updating database weights\n");
	}
};

CExperiment::~CExperiment(void) {
	delete wu;
	pthread_mutex_destroy(&lock_);
}


pthread_mutex_t global_clop_lock;
pthread_mutex_t clop_seed_lock;
std::unordered_map<int, CExperiment *> clops;

/* Locks mutex of clop object and returns it. */
void *clop_load(int id) {
	pthread_mutex_lock(&global_clop_lock);
	printf("actually loading\n");
	auto it = clops.find(id);
	if (it != clops.end()) {
		it->second->lock();
		pthread_mutex_unlock(&global_clop_lock);
		return (void *)it->second;
	}

	pthread_mutex_lock(&db_lock);

	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
		"SELECT spsa\n"
		"FROM tests\n"
		"WHERE id = ?;",
		-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, id);

	if (sqlite3_step(stmt) == SQLITE_DONE) {
		sqlite3_finalize(stmt);
		pthread_mutex_unlock(&db_lock);
		pthread_mutex_unlock(&global_clop_lock);
		return NULL;
	}

	std::vector<std::unique_ptr<CParameter>> params;
	CParameterCollection paramcol;

	cJSON *spsa = cJSON_Parse((const char *)sqlite3_column_text(stmt, 0));
	cJSON *param = NULL;
	cJSON_ArrayForEach(param, spsa) {
		const char *name = param->string;
		double min = cJSON_GetObjectItemCaseSensitive(param, "min")->valuedouble;
		double max = cJSON_GetObjectItemCaseSensitive(param, "max")->valuedouble;
		params.push_back(std::unique_ptr<CParameter>(new CLinearParameter(name, min, max)));
		paramcol.Add(*params.back());
	}

	cJSON_Delete(spsa);

	sqlite3_finalize(stmt);

	pthread_mutex_unlock(&db_lock);

	CExperiment *cexp = new CExperiment(std::move(params), std::move(paramcol), std::random_device{}());
	printf("created experiment\n");
	cexp->lock();
	cexp->reg.SetAutoLocalize(false);
	clops[id] = cexp;
	pthread_mutex_unlock(&global_clop_lock);

	pthread_mutex_lock(&db_lock);
	sqlite3_prepare_v2(db,
		"SELECT spsa, w, d, l\n"
		"FROM games\n"
		"WHERE testid = ?\n"
			"AND donetime IS NOT NULL\n"
			"AND w + d + l = 2\n"
		"ORDER BY starttime ASC;",
		-1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, id);

	int i = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		if (i++ % 1000 == 0)
			printf("looping\n");
		cJSON *spsa = cJSON_Parse((const char *)sqlite3_column_text(stmt, 0));

		std::vector<double> v = cexp->json_to_point(spsa);

		int w = sqlite3_column_int(stmt, 1);
		int d = sqlite3_column_int(stmt, 2);
		int l = sqlite3_column_int(stmt, 3);

		for (int i = 0; i < w; i++)
			cexp->results.AddSample(&v[0], COutcome::Win);
		for (int i = 0; i < d; i++)
			cexp->results.AddSample(&v[0], COutcome::Draw);
		for (int i = 0; i < l; i++)
			cexp->results.AddSample(&v[0], COutcome::Loss);

		cJSON_Delete(spsa);
	}
	printf("done looping (%d)\n", i);

	sqlite3_finalize(stmt);
	pthread_mutex_unlock(&db_lock);

	cexp->reg.SetAutoLocalize(true);
	printf("computing weights\n");
	cexp->reg.ComputeLocalWeights();
	printf("computed weights\n");

	cexp->reg.SetRefreshRate(0.1);
	double DrawElo = 100.0;
	double DrawRating = DrawElo * log(10.0) / 400.0;
	cexp->reg.SetDrawRating(DrawRating);
	cexp->reg.SetLocalizationHeight(3.0);

	cexp->reg.SetMaxWeightIterations(7);
	cexp->pf.SetPriorStrength(1e-2);

	cexp->wu = new CWeightUpdater(*cexp, id);
	printf("created cweightupdater\n");

	return (void *)cexp;
}

void clop_return(void *clop) {
	if (!clop)
		return;
	((CExperiment *)clop)->unlock();
}

void clop_unload(int id) {
	pthread_mutex_lock(&global_clop_lock);
	auto it = clops.find(id);
	if (it == clops.end()) {
		pthread_mutex_unlock(&global_clop_lock);
		return;
	}
	/* If we can lock and unlock while having global_clop_lock,
	 * that means that noone else can currently be using this clop object.
	 */
	it->second->lock();
	it->second->unlock();
	delete it->second;
	clops.erase(it);
	pthread_mutex_unlock(&global_clop_lock);
}

void clop_clear(void) {
	for (auto &kv : clops)
		delete kv.second;
	clops.clear();
}

void clop_init(void) {
	pthread_mutex_init(&global_clop_lock, NULL);
	pthread_mutex_init(&clop_seed_lock, NULL);
}

void clop_term(void) {
	clop_clear();
	pthread_mutex_destroy(&global_clop_lock);
	pthread_mutex_destroy(&clop_seed_lock);
}

cJSON *clop_next_sample(void *e, int *seed, double *weight) {
	CExperiment *cexp = (CExperiment *)e;

	int Seed = cexp->results.GetSamples();
	cexp->results.AddSample(cexp->sp.NextSample(Seed));

	Seed = cexp->results.GetSamples();
	cexp->results.Reserve(Seed + 1);
	cexp->results.AddSample(cexp->results.GetSample(Seed - 1));

	*seed = Seed - 1;
	*weight = cexp->reg.GetWeight(cexp->results.GetSample(Seed - 1));

	for (int i = 0; i < 2; i++) {
		std::cout << cexp->results.GetSample(Seed - 1)[i] << std::endl;
	}

	return cexp->point_to_json(cexp->results.GetSample(Seed - 1));
}

std::vector<std::pair<std::pair<int, int>, int>> seeds;

void clop_store_seed(int id, int task_id, int seed) {
	pthread_mutex_lock(&clop_seed_lock);
	if (seeds.size() >= 1000)
		seeds.erase(seeds.begin());
	seeds.push_back({{id, task_id}, seed});
	pthread_mutex_unlock(&clop_seed_lock);
}

int clop_pop_seed(int id, int task_id) {
	int seed = -1;
	pthread_mutex_lock(&clop_seed_lock);
	for (size_t i = 0; i < seeds.size(); i++) {
		if (seeds[i].first.first == id && seeds[i].first.second == task_id) {
			seed = seeds[i].second;
			seeds.erase(seeds.begin() + i);
			break;
		}
	}
	pthread_mutex_unlock(&clop_seed_lock);
	return seed;
}

void clop_add_outcome(void *e, int seed, int w, int d, int l) {
	CExperiment *cexp = (CExperiment *)e;

	for (int i = 0; i < w; i++) {
		cexp->results.AddOutcome(seed, COutcome::Win);
		seed += 1;
	}
	for (int i = 0; i < d; i++) {
		cexp->results.AddOutcome(seed, COutcome::Draw);
		seed += 1;
	}
	for (int i = 0; i < l; i++) {
		cexp->results.AddOutcome(seed, COutcome::Loss);
		seed += 1;
	}
}

cJSON *clop_get_mean(void *e) {
	CExperiment *cexp = (CExperiment *)e;
	int dim = cexp->reg.GetPF().GetDimensions();
	std::vector<double> mean(dim);
	cexp->me.MaxParameter(&mean[0]);
	return cexp->point_to_json(&mean[0]);
}

cJSON *clop_get_max(void *e) {
	CExperiment *cexp = (CExperiment *)e;
	int dim = cexp->reg.GetPF().GetDimensions();
	std::vector<double> maximum(dim);
	if (!cexp->reg.GetPF().GetMax(cexp->reg.MAP(), &maximum[0]))
		return NULL;
	return cexp->point_to_json(&maximum[0]);
}
