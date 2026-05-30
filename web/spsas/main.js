import { formatDate } from '/util.js';

async function getData() {
	const response = await fetch('https://jalagaoi.se/testbit/spsa');
	const json = await response.json();
	return await json;
}

function truncate(text, n) {
	if (text.length > n)
		return text.substring(0, n - 3) + '...';
	return text;
}

document.getElementById('testbutton').addEventListener('click', () => {
	window.location.href = '/';
});
document.getElementById('clopbutton').addEventListener('click', () => {
	window.location.href = '/clops';
});

const table = document.getElementById('testtable');
table.addEventListener('click', function(event) {
	const row = event.target.closest('tr');
	if (row && row.dataset.id) {
		window.location.href = `/spsa?id=${row.dataset.id}`;
	}
});
table.addEventListener('mousedown', function(event) {
	if (event.button === 1) event.preventDefault();
});
table.addEventListener('auxclick', function(event) {
	const row = event.target.closest('tr');
	if (row && row.dataset.id && event.button === 1)
		window.open(`/spsa?id=${row.dataset.id}`);
});

const thead = table.createTHead();
const headerRow = thead.insertRow();
const headers = ['Description', 'Status', 'N', 'Alpha', 'Gamma', 'A', 'TC', 'Queue Timestamp', 'Start Timestamp', 'Done Timestamp'];

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
		const tests = data.tests;
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
			const desc = row.cells[0];
			desc.textContent = truncate(test.description, 33);
			const stat = row.cells[1];
			stat.textContent = test.status;

			const N = row.cells[2];
			N.textContent = test.N;
			N.style.textAlign = 'right';

			const alpha = row.cells[3];
			alpha.textContent = test.alpha;
			alpha.style.textAlign = 'right';

			const gamma = row.cells[4];
			gamma.textContent = test.gamma;
			gamma.style.textAlign = 'right';

			const A = row.cells[5];
			A.textContent = test.A;
			A.style.textAlign = 'right';

			const tc = row.cells[6];
			tc.textContent = test.tc;
			tc.style.textAlign = 'center';

			const queue = row.cells[7];
			if (test.queuetime) {
				queue.textContent = formatDate(test.queuetime);
				queue.style.textAlign = 'center';
			}
			else
				queue.textContent = '';
			const start = row.cells[8];
			if (test.starttime) {
				start.textContent = formatDate(test.starttime);
				start.style.textAlign = 'center';
			}
			else
				start.textContent = '';
			const done = row.cells[9];
			if (test.donetime) {
				done.textContent = formatDate(test.donetime);
				done.style.textAlign = 'center';
			}
			else
				done.textContent = '';
		})
	})
}
