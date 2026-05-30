async function getData() {
	const response = await fetch('http://192.168.1.214:3333/clop');
	console.log(response)
	const json = await response.json();
	return await json;
}

function formatDate(unixepoch) {
	if (!unixepoch) {
		return '';
	}
	const date = new Date(unixepoch * 1000);
	const year = String(date.getFullYear());
	const month = String(date.getMonth() + 1);
	const day = String(date.getDate());
	const hour = String(date.getHours());
	const minute = String(date.getMinutes());
	const second = String(date.getSeconds());
	return `${year}-${month.padStart(2, '0')}-${day.padStart(2, '0')} ${hour.padStart(2, '0')}:${minute.padStart(2, '0')}:${second.padStart(2, '0')}`;
}

function formatElo(elo, pm) {
	if (elo != null) {
		var elotext = elo.toFixed(3).toString();
		if (pm != null) {
			var pmtext = pm.toFixed(3).toString();
			if (elotext.length < pmtext.length)
				elotext = ' '.repeat(pmtext.length - elotext.length) + elotext + '\u00B1' + pmtext;
			else
				elotext += '\u00B1' + pmtext + ' '.repeat(elotext.length - pmtext.length);
		}
		else {
			var split = elotext.split('.');
			var beforedot = split[0].length;
			var afterdot = split[1].length;
			if (beforedot < afterdot)
				elotext = ' '.repeat(afterdot - beforedot) + elotext;
			else
				elotext += ' '.repeat(beforedot - afterdot);
		}
	}
	else
		elotext = '';
	return elotext;
}

function truncate(text, n) {
	if (text.length > n)
		return text.substring(0, n - 3) + '...';
	return text;
}

function totests() {
	window.location.href = '/';
}

const table = document.getElementById('testtable');
table.addEventListener('click', function(event) {
	const row = event.target.closest('tr');
	if (row && row.dataset.id) {
		window.location.href = `/clop?id=${row.dataset.id}`;
	}
});
table.addEventListener('mousedown', function(event) {
	if (event.button === 1) event.preventDefault();
});
table.addEventListener('auxclick', function(event) {
	const row = event.target.closest('tr');
	if (row && row.dataset.id && event.button === 1)
		window.open(`/clop?id=${row.dataset.id}`);
});

const thead = table.createTHead();
const headerRow = thead.insertRow();
const headers = ['Description', 'Status', 'N', 'Elo (All)', 'Elo (Weighted)', 'Elo (Central)', 'TC', 'Queue Timestamp', 'Start Timestamp', 'Done Timestamp'];

headers.forEach(text => {
	const th = document.createElement('th');
	th.textContent = text;
	headerRow.appendChild(th);
})

let intervalId;

function startUpdating() {
	if (intervalId)
		clearInterval(intervalId);
	intervalId = setInterval(drawTable, 10000);
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
		drawTable();
		startUpdating();
	}
});

drawTable();
startUpdating();

function drawTable() {
	console.log('redrawing');
	getData().then(data => {
		if (data.message != 'ok')
			throw new Error('Bad request.');
		tests = data.tests;
		tests.sort((a, b) => {
			if (a.donetime && !b.donetime)
				return true;
			if (b.donetime && !a.donetime)
				return false;
			if (a.donetime && b.donetime)
				return a.donetime < b.donetime;
			return a.queuetime < b.queuetime;
		});
		tests.forEach((test, index) => {
			var row;
			if (index + 1 < table.rows.length)
				row = table.rows[index + 1];
			else {
				row = table.insertRow();
				for (let i = 0; i < 10; i++)
					row.insertCell();
			}

			row.classList.add('clickable-row');
			row.dataset.id = test.id;
			desc = row.cells[0];
			desc.textContent = truncate(test.description, 33);
			stat = row.cells[1];
			stat.textContent = test.status;

			N = row.cells[2];
			N.textContent = test.N;
			N.style.textAlign = 'right';

			eloall = row.cells[3];
			eloall.textContent = formatElo(test.eloall, test.pmall);
			eloall.style.textAlign = 'right';

			eloweighted = row.cells[4];
			eloweighted.textContent = formatElo(test.eloweighted, test.pmweighted);
			eloweighted.style.textAlign = 'right';

			elocentral = row.cells[5];
			elocentral.textContent = formatElo(test.elocentral, test.pmcentral);
			elocentral.style.textAlign = 'right';

			tc = row.cells[6];
			tc.textContent = test.tc;
			tc.style.textAlign = 'center';

			queue = row.cells[7];
			if (test.queuetime) {
				queue.textContent = formatDate(test.queuetime);
				queue.style.textAlign = 'center';
			}
			else
				queue.textContent = '';
			start = row.cells[8];
			if (test.starttime) {
				start.textContent = formatDate(test.starttime);
				start.style.textAlign = 'center';
			}
			else
				start.textContent = '';
			done = row.cells[9];
			if (test.donetime) {
				done.textContent = formatDate(test.donetime);
				done.style.textAlign = 'center';
			}
			else
				done.textContent = '';
		})
	})
}
