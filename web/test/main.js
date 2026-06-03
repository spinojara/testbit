import { formatDate, formatPatch, parseEscapeCodes } from '/util.js';

async function getData(id) {
	const response = await fetch(`https://jalagaoi.se/testbit/test/${id}?delta=1800`);
	const json = await response.json();
	return await json;
}

async function putData(endpoint, credentials) {
	const response = await fetch(endpoint, {
		method: 'PUT',
		headers: {
			'Authorization': 'Basic ' + credentials
		}
	});
	const json = await response.json();
	return await json;
}

function redirectHome() {
	window.location.href = '/';
}

function truncate(text, n) {
	if (text.length > n)
		return text.substring(0, n - 3) + '...';
	return text;
}

function eta(type, N, alpha, beta, llr, eloe, pm, gametimeavg) {
	var N_needed;
	if (type == 'sprt') {
		const A = Math.log(beta / (1.0 - alpha));
		const B = Math.log((1.0 - beta) / alpha);
		const target = llr < 0.0 ? A : B;
		N_needed = N * (target / llr - 1.0);
	}
	else {
		N_needed = N * ((pm / eloe) ** 2 - 1.0);
	}
	return Date.now() / 1000 + N_needed * gametimeavg;
}

function getid() {
	const params = new URLSearchParams(window.location.search);
	const id = params.get('id') || null;
	return id;
}

const id = getid();

if (!id)
	redirectHome();

const table = document.getElementById('testtable');
const buttondiv = document.getElementById('buttondiv');
const prepatch = document.getElementById('patch')
const preerrorlog = document.getElementById('errorlog')
const thead = table.createTHead();
const headerRow = thead.insertRow();
let shouldUpdate = true;

getData(id).then(data => {
	if (data.message != 'ok')
		redirectHome();
	const test = data.test;
	const headers = ['Description', 'Type', 'Status', 'Elo', 'Trinomial', 'Pentanomial'];

	if (test.type == 'sprt')
		headers.push(...['LLR', '(A, B)', '\u03B1', '\u03B2', 'Elo 0', 'Elo 1']);
	else
		headers.push('Elo Error');

	headers.push(...['TC', 'Commit ID'])
	shouldUpdate = test.donetime == null;

	if (test.starttime)
		headers.push('Start Timestamp');
	else if (test.queuetime)
		headers.push('Queue Timestamp');

	if (test.donetime)
		headers.push('Done Timestamp');
	else
		headers.push('ETA');

	headers.forEach(text => {
		const th = document.createElement('th');
		th.textContent = text;
		headerRow.appendChild(th);
	})

	const row = table.insertRow();
	row.dataset.id = test.id;
	const desc = row.insertCell();
	desc.textContent = truncate(test.description, 33);
	const type = row.insertCell();
	type.textContent = test.type;
	type.style.textAlign = 'center';
	const stat = row.insertCell();
	stat.textContent = test.status;
	const elo = row.insertCell();
	if (test.elo != null) {
		elo.textContent = test.elo.toFixed(3);
		if (test.pm != null)
			elo.textContent += '\u00B1' + test.pm.toFixed(3);
	}
	elo.style.textAlign = 'center';
	const trinomial = row.insertCell();
	trinomial.textContent = test.t0 + '-' + test.t1 + '-' + test.t2;
	trinomial.style.textAlign = 'center';
	const pentanomial = row.insertCell();
	pentanomial.textContent = test.p0 + '-' + test.p1 + '-' + test.p2 + '-' + test.p3 + '-' + test.p4;
	pentanomial.style.textAlign = 'center';
	var index = 8;
	if (test.type == 'sprt') {
		const llr = row.insertCell();
		if (test.llr != null)
			llr.textContent = test.llr.toFixed(3);

		const AB = row.insertCell();
		AB.textContent = "(" + Math.log(test.beta / (1.0 - test.alpha)).toFixed(3) + ", " + Math.log((1.0 - test.beta) / test.alpha).toFixed(3) + ")";
		AB.style.textAlign = 'center';

		const alpha = row.insertCell();
		alpha.textContent = test.alpha;
		alpha.style.textAlign = 'right';

		const beta = row.insertCell();
		beta.textContent = test.beta;
		beta.style.textAlign = 'right';

		const elo0 = row.insertCell();
		elo0.textContent = test.elo0.toFixed(3);
		elo0.style.textAlign = 'right';

		const elo1 = row.insertCell();
		elo1.textContent = test.elo1.toFixed(3);
		elo1.style.textAlign = 'right';
		index = 13;
	}
	else {
		const eloe = row.insertCell();
		eloe.textContent = test.eloe.toFixed(3);
		eloe.style.textAlign = 'right';
	}

	const tc = row.insertCell();
	tc.textContent = test.tc;
	tc.style.textAlign = 'center';

	const commit = row.insertCell();
	commit.textContent = truncate(test.commit, 12);
	commit.style.textAlign = 'center';

	const start = row.insertCell();
	const done = row.insertCell();
	start.style.textAlign = 'center';
	done.style.textAlign = 'center';
	const startheader = table.tHead.rows[0].cells[index + 1];
	const doneheader = table.tHead.rows[0].cells[index + 2];
	if (test.donetime) {
		doneheader.textContent = 'Done Timestamp';
		done.textContent = formatDate(test.donetime);
	}
	else {
		doneheader.textContent = 'ETA';
		const N = (test.t0 + test.t1 + test.t2) / 2;
		if (test.gametimeavg >= 0 && N > 0 && ((test.type == 'elo' && test.pm > 0 && test.eloe > 0) || (test.type == 'sprt' && test.llr != 0)))
			done.textContent = formatDate(eta(test.type, N, test.alpha, test.beta, test.llr, test.eloe, test.pm, test.gametimeavg));
		else
			done.textContent = '';
	}
	if (test.starttime) {
		startheader.textContent = 'Start Timestamp';
		start.textContent = formatDate(test.starttime);
	}
	else if (test.queuetime) {
		startheader.textContent = 'Queue Timestamp';
		start.textContent = formatDate(test.queuetime);
	}

	prepatch.innerHTML = formatPatch(test.patch);
	preerrorlog.innerHTML = parseEscapeCodes(test.errorlog);
	if (!test.errorlog)
		preerrorlog.style.display = 'none';

	const button = document.getElementById('actionbutton');
	button.onclick = promptpassword;
	if (test.status == 'cancelled')
		button.textContent = 'Resume';
	else if (test.status == 'running' || test.status == 'building' || test.status == 'queued')
		button.textContent = 'Cancel';
	else
		button.textContent = 'Requeue';
});

let intervalId;

function startUpdating() {
	if (intervalId)
		clearInterval(intervalId);
	intervalId = setInterval(updateTable, 10000);
}

function stopUpdating() {
	if (intervalId) {
		clearInterval(intervalId);
		intervalId = null;
	}
}

document.addEventListener('visibilitychange', () => {
	if (document.hidden) {
		stopUpdating();
	}
	else {
		updateTable();
		startUpdating();
	}
});

startUpdating();

function promptpassword() {
	const passwordprompt = document.getElementById('passwordprompt');
	passwordprompt.classList = '';
}

document.getElementById('buttoncontinue').addEventListener('click', () => {
	const passwordprompt = document.getElementById('passwordprompt');
	passwordprompt.classList = 'hidden';
	const passwordinput = document.getElementById('passwordinput');
	const password = passwordinput.value;
	passwordinput.value = '';
	const credentials = btoa(':' + password);

	const button = document.getElementById('actionbutton');
	var endpoint = 'https://jalagaoi.se/testbit/test/';
	switch (button.textContent) {
	case 'Resume':
		endpoint += 'resume/';
		break;
	case 'Cancel':
		endpoint += 'cancel/';
		break;
	case 'Requeue':
		endpoint += 'requeue/';
		break;
	default:
		return;
	}
	endpoint += getid();
	putData(endpoint, credentials).then(data => {
		if (data.message == 'wrong password') {
			const wrongpassword = document.getElementById('wrongpassword');
			wrongpassword.classList = '';
		}
		shouldUpdate = true;
		updateTable();
	});
});

document.getElementById('buttonwrongpasswordok').addEventListener('click', () => {
	const wrongpassword = document.getElementById('wrongpassword');
	wrongpassword.classList = 'hidden';
});

document.getElementById('buttoncancel').addEventListener('click', () => {
	const passwordprompt = document.getElementById('passwordprompt');
	passwordprompt.classList = 'hidden';
	const passwordinput = document.getElementById('passwordinput');
	passwordinput.value = '';
});

function updateTable() {
	if (!shouldUpdate)
		return;
	getData(id).then(data => {
		if (data.message != 'ok')
			redirectHome();
		const test = data.test;
		shouldUpdate = test.donetime == null;
		const statusCell = table.rows[1].cells[2];
		statusCell.textContent = test.status;
		const elo = table.rows[1].cells[3];
		if (test.elo != null) {
			elo.textContent = test.elo.toFixed(3);
			if (test.pm != null)
				elo.textContent += '\u00B1' + test.pm.toFixed(3);
		}
		const trinomial = table.rows[1].cells[4];
		trinomial.textContent = test.t0 + '-' + test.t1 + '-' + test.t2;
		const pentanomial = table.rows[1].cells[5];
		pentanomial.textContent = test.p0 + '-' + test.p1 + '-' + test.p2 + '-' + test.p3 + '-' + test.p4;
		var index = 8;
		if (test.type == 'sprt') {
			const llr = table.rows[1].cells[6];
			if (test.llr != null)
				llr.textContent = test.llr.toFixed(3);
			index = 13;
		}
		const commit = table.rows[1].cells[index];
		commit.textContent = truncate(test.commit, 12);
		const start = table.rows[1].cells[index + 1];
		const done = table.rows[1].cells[index + 2];
		const startheader = table.tHead.rows[0].cells[index + 1];
		const doneheader = table.tHead.rows[0].cells[index + 2];
		if (test.donetime) {
			doneheader.textContent = 'Done Timestamp';
			done.textContent = formatDate(test.donetime);
		}
		else {
			doneheader.textContent = 'ETA';
			const N = (test.t0 + test.t1 + test.t2) / 2;
			if (test.gametimeavg >= 0 && N > 0 && ((test.type == 'elo' && test.pm > 0 && test.eloe > 0) || (test.type == 'sprt' && test.llr != 0)))
				done.textContent = formatDate(eta(test.type, N, test.alpha, test.beta, test.llr, test.eloe, test.pm, test.gametimeavg));
			else
				done.textContent = '';
		}
		if (test.starttime) {
			startheader.textContent = 'Start Timestamp';
			start.textContent = formatDate(test.starttime);
		}
		else if (test.queuetime) {
			startheader.textContent = 'Queue Timestamp';
			start.textContent = formatDate(test.queuetime);
		}
		const button = document.getElementById('actionbutton');
		if (test.status == 'cancelled')
			button.textContent = 'Resume';
		else if (test.status == 'running' || test.status == 'building' || test.status == 'queued')
			button.textContent = 'Cancel';
		else
			button.textContent = 'Requeue';
	});
}
