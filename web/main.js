async function getData() {
	const response = await fetch('https://jalagaoi.se:2718/test');
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

const params = new URLSearchParams(window.location.search);
const testStatus = params.get('status') || 'all';

const table = document.getElementById('testtable');
const thead = table.createTHead();
const headerRow = thead.insertRow();

table.addEventListener('click', function(event) {
	const row = event.target.closest('tr');
	if (row && row.dataset.id) {
		window.location.href = `/test?id=${row.dataset.id}`;
		console.log(row);
	}
});

const headers = ['Description', 'Type', 'Status', 'Elo', 'Trinomial', 'Pentanomial', 'LLR'];

if (testStatus != 'error' && testStatus != 'done' && testStatus != 'cancelled')
	headers.push(...['Queue Timestamp', 'Start Timestamp']);
if (testStatus != 'running')
	headers.push('Done Timestamp');

headers.forEach(text => {
	const th = document.createElement('th');
	th.textContent = text;
	headerRow.appendChild(th);
})

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
	tests.forEach(test => {
		const row = table.insertRow();
		row.classList.add('clickable-row');
		row.dataset.id = test.id;
		desc = row.insertCell();
		var description = test.description;
		if (description.length > 33)
			description = description.slice(0, 30).trimRight() + "...";
		desc.textContent = description;
		type = row.insertCell();
		type.textContent = test.type;
		type.style.textAlign = 'center';
		stat = row.insertCell();
		stat.textContent = test.status;
		elo = row.insertCell();
		if (test.elo) {
			elo.textContent = test.elo.toFixed(3);
			if (test.pm)
				elo.textContent += '\u00B1' + test.pm.toFixed(3);
		}
		elo.style.textAlign = 'center';
		trinomial = row.insertCell();
		trinomial.textContent = test.t0 + '-' + test.t1 + '-' + test.t2;
		trinomial.style.textAlign = 'center';
		pentanomial = row.insertCell();
		pentanomial.textContent = test.p0 + '-' + test.p1 + '-' + test.p2 + '-' + test.p3 + '-' + test.p4;
		pentanomial.style.textAlign = 'center';
		llr = row.insertCell();
		llr.style.textAlign = 'right';
		if (test.llr)
			llr.textContent = test.llr.toFixed(3);
		if (testStatus != 'error' && testStatus != 'done' && testStatus != 'cancelled') {
			queue = row.insertCell();
			queue.textContent = formatDate(test.queuetime);
			queue.style.textAlign = 'center';
			start = row.insertCell();
			start.textContent = formatDate(test.starttime);
			start.style.textAlign = 'center';
		}
		if (testStatus != 'running') {
			done = row.insertCell();
			done.textContent = formatDate(test.donetime);
			done.style.textAlign = 'center';
		}
	})
})

