import { formatDate, formatElo } from '/util.js';

async function getData() {
	const response = await fetch('https://jalagaoi.se/testbit/test?delta=1800');
	const json = await response.json();
	return await json;
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
		console.log(target);
		N_needed = N * (target / llr - 1.0);
	}
	else {
		N_needed = N * ((pm / eloe) ** 2 - 1.0);
	}
	return Date.now() / 1000 + N_needed * gametimeavg;
}

document.getElementById('spsabutton').addEventListener('click', () => {
	window.location.href = '/spsas';
});
document.getElementById('clopbutton').addEventListener('click', () => {
	window.location.href = '/clops';
});

const table = document.getElementById('testtable');
table.addEventListener('click', function(event) {
	const row = event.target.closest('tr');
	if (row && row.dataset.id) {
		window.location.href = `/test?id=${row.dataset.id}`;
	}
});
table.addEventListener('mousedown', function(event) {
	if (event.button === 1) event.preventDefault();
});
table.addEventListener('auxclick', function(event) {
	const row = event.target.closest('tr');
	if (row && row.dataset.id && event.button === 1)
		window.open(`/test?id=${row.dataset.id}`);
});

const thead = table.createTHead();
const headerRow = thead.insertRow();
const headers = ['Description', 'Type', 'Status', 'Elo', 'Trinomial', 'Pentanomial', 'LLR', 'Queue Timestamp', 'Start Timestamp', 'Done Timestamp'];

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
		if (data.message != 'ok') {
			console.log(data.message);
			throw new Error('Bad request.');
		}
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
			const type = row.cells[1];
			type.textContent = test.type;
			type.style.textAlign = 'center';
			const stat = row.cells[2];
			stat.textContent = test.status;
			const elo = row.cells[3];
			elo.textContent = formatElo(test.elo, test.pm)
			elo.style.textAlign = 'center';
			elo.style.whiteSpace = 'pre';
			const trinomial = row.cells[4];
			trinomial.textContent = test.t0 + '-' + test.t1 + '-' + test.t2;
			var righttext = test.t0.toString().length;
			var lefttext = test.t2.toString().length;
			if (righttext < lefttext)
				trinomial.textContent = ' '.repeat(lefttext - righttext) + trinomial.textContent;
			else
				trinomial.textContent += ' '.repeat(righttext - lefttext);
			trinomial.style.textAlign = 'center';
			trinomial.style.whiteSpace = 'pre';
			const pentanomial = row.cells[5];
			pentanomial.textContent = test.p0 + '-' + test.p1 + '-' + test.p2 + '-' + test.p3 + '-' + test.p4;
			var righttext = test.p0.toString().length + test.p1.toString().length;
			var lefttext = test.p3.toString().length + test.p4.toString().length;
			if (righttext < lefttext)
				pentanomial.textContent = ' '.repeat(lefttext - righttext) + pentanomial.textContent;
			else
				pentanomial.textContent += ' '.repeat(righttext - lefttext);
			pentanomial.style.textAlign = 'center';
			pentanomial.style.whiteSpace = 'pre';
			const llr = row.cells[6];
			llr.style.textAlign = 'right';
			if (test.llr != null)
				llr.textContent = test.llr.toFixed(3);
			else
				llr.textContent = '';
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
			done.style.textAlign = 'center';
			done.style.whiteSpace = 'pre';
			const N = (test.t0 + test.t1 + test.t2) / 2;
			if (test.donetime) {
				done.textContent = formatDate(test.donetime);
			}
			else if (test.gametimeavg >= 0 && N > 0 && ((test.type == 'elo' && test.pm > 0 && test.eloe > 0) || (test.type == 'sprt' && test.llr != 0))) {
				done.textContent = '      ' + formatDate(eta(test.type, N, test.alpha, test.beta, test.llr, test.eloe, test.pm, test.gametimeavg)) + ' (ETA)';
			}
			else {
				done.textContent = '';
			}
		})
	})
}
