import { parseEscapeCodes, formatDate, formatPatch, formatElo } from '/util.js';

async function getData(id) {
	const response = await fetch(`https://jalagaoi.se/testbit/clop/${id}`);
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

function getid() {
	const params = new URLSearchParams(window.location.search);
	const id = params.get('id') || null;
	return id;
}

var id = getid();

if (!id)
	redirectHome();

const table = document.getElementById('testtable');
const buttondiv = document.getElementById('buttondiv');
const prepatch = document.getElementById('patch');
const preerrorlog = document.getElementById('errorlog');
const paramtable = document.getElementById('paramtable');
const tHead = table.createTHead();
const paramTHead = paramtable.createTHead();
const headerRow = tHead.insertRow();
const paramHeaderRow = paramTHead.insertRow();
const canvas = document.getElementById('canvas');
const ctx = canvas.getContext('2d');
const selectx = document.getElementById('selectx');
const selecty = document.getElementById('selecty')
var spsahistory = null;
var spsa = null;
let shouldUpdate = true;

function drawImage() {
	if (!spsa || !spsahistory)
		return;

	ctx.clearRect(0, 0, canvas.width, canvas.height)

	var axisX = '';
	var axisY = '';
	var minX = 0.0;
	var maxX = 1.0;
	var minY = 0.0;
	var maxY = 1.0;
	if (selectx.selectedIndex !== 0) {
		axisX = selectx.value;
		minX = spsa[axisX].min;
		maxX = spsa[axisX].max;
	}
	if (selecty.selectedIndex !== 0) {
		axisY = selecty.value;
		minY = spsa[axisY].min;
		maxY = spsa[axisY].max;
	}

	var i = 1;
	var length = spsahistory.length + 1;
	for (const point of spsahistory) {
		var weight = point._weight ** 2;
		var phase = i / length;

		if (axisX !== '')
			var x = Math.round((canvas.width - 1) * (point[axisX] - minX) / (maxX - minX));
		else
			var x = Math.round((canvas.width - 1) * phase);
		if (axisY !== '')
			var y = Math.round((canvas.height - 1) * (maxY - point[axisY]) / (maxY - minY));
		else
			var y = Math.round((canvas.height - 1) * (1.0 - phase));

		var r = 0;
		var g = 0;
		var b = 0;
		switch (point._score) {
			case 0.0:
				r = 255;
				break;
			case 0.25:
				r = 255;
				g = 100;
				break;
			case 0.5:
				r = 255;
				g = 165;
				break;
			case 0.75:
				r = 150;
				g = 255;
				break;
			case 1.0:
				g = 255;
				break;
			default:
				r = 40;
				g = 40;
				b = 40;
		}

		r = weight * r + (1.0 - weight) * 40;
		g = weight * g + (1.0 - weight) * 40;
		b = weight * b + (1.0 - weight) * 40;

		ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;
		ctx.fillRect(x, y, 1, 1);
		ctx.fill();
		i += 1;
	}

}

getData(id).then(data => {
	if (data.message != 'ok')
		redirectHome();
	const test = data.test;
	const headers = ['Description', 'Status', 'N', 'Elo (All)', 'Elo (Weighted)', 'Elo (Central)', 'TC', 'Commit ID'];

	shouldUpdate = test.donetime == null;

	if (test.donetime)
		headers.push('Done Timestamp');
	else if (test.starttime)
		headers.push('Start Timestamp');
	else if (test.queuetime)
		headers.push('Queue Timestamp');

	headers.forEach(text => {
		const th = document.createElement('th');
		th.textContent = text;
		headerRow.appendChild(th);
	})

	const row = table.insertRow();
	row.dataset.id = test.id;
	const desc = row.insertCell();
	desc.textContent = truncate(test.description, 33);
	const stat = row.insertCell();
	stat.textContent = test.status;

	const N = row.insertCell();
	N.textContent = test.N;
	N.style.textAlign = 'right';

	const eloall = row.insertCell();
	eloall.textContent = formatElo(test.eloall, test.pmall);
	eloall.style.textAlign = 'right';

	const eloweighted = row.insertCell();
	eloweighted.textContent = formatElo(test.eloweighted, test.pmweighted);
	eloweighted.style.textAlign = 'right';

	const elocentral = row.insertCell();
	elocentral.textContent = formatElo(test.elocentral, test.pmcentral);
	elocentral.style.textAlign = 'right';

	const tc = row.insertCell();
	tc.textContent = test.tc;
	tc.style.textAlign = 'center';

	const commit = row.insertCell();
	commit.textContent = truncate(test.commithash, 12);
	commit.style.textAlign = 'center';

	const time = row.insertCell();
	time.style.textAlign = 'center';
	const timeheader = table.tHead.rows[0].cells[8];
	if (test.donetime) {
		timeheader.textContent = 'Done Timestamp';
		time.textContent = formatDate(test.donetime);
	}
	else if (test.starttime) {
		timeheader.textContent = 'Start Timestamp';
		time.textContent = formatDate(test.starttime);
	}
	else {
		timeheader.textContent = 'Queue Timestamp';
		time.textContent = formatDate(test.queuetime);
	}

	prepatch.innerHTML = formatPatch(test.patch);
	if (!test.patch)
		prepatch.style.display = 'none';
	preerrorlog.innerHTML = parseEscapeCodes(test.errorlog);
	if (!test.errorlog)
		preerrorlog.style.display = 'none';

	const button = document.getElementById('actionbutton');
	button.onclick = promptpassword;
	if (test.status == 'cancelled')
		button.textContent = 'Resume';
	else
		button.textContent = 'Cancel';

	const paramheaders = ['Parameter', 'Mean', 'Maximum', 'Min', 'Max']
	paramheaders.forEach(text => {
		const th = document.createElement('th');
		th.textContent = text;
		paramHeaderRow.appendChild(th);
	});

	selectx.appendChild(new Option('Time'));
	selecty.appendChild(new Option('Time'));

	for (const [key, param] of Object.entries(test.spsa)) {
		const row2 = paramtable.insertRow();
		row2.dataset.name = key;

		const name = row2.insertCell();
		name.textContent = key;

		const mean = row2.insertCell();
		if (param.mean != null) {
			mean.textContent = param.mean.toFixed(3);
			mean.style.textAlign = 'right';
		}

		const maximum = row2.insertCell();
		if (param.maximum != null) {
			maximum.textContent = param.maximum.toFixed(3);
			maximum.style.textAlign = 'right';
		}

		const min = row2.insertCell();
		min.textContent = param.min.toFixed(3);
		min.style.textAlign = 'right';

		const max = row2.insertCell();
		max.textContent = param.max.toFixed(3);
		max.style.textAlign = 'right';

		selectx.appendChild(new Option(key));
		selecty.appendChild(new Option(key));
	}

	selectx.selectedIndex = 1 - (test.spsa.length == 1);
	selecty.selectedIndex = 2 - (test.spsa.length == 1);

	ctx.imageSmoothingEnabled = false;
	ctx.mozImageSmoothingEnabled = false;
	ctx.webkitImageSmoothingEnabled = false;

	spsahistory = test.spsahistory;
	spsa = test.spsa;

	drawImage();
	selectx.addEventListener('change', (e) => {
		drawImage();
	});
	selecty.addEventListener('change', (e) => {
		drawImage();
	});
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
		var test = data.test;
		var shouldUpdate = test.donetime == null;

		const statusCell = table.rows[1].cells[1];
		statusCell.textContent = test.status;

		const NCell = table.rows[1].cells[2];
		NCell.textContent = test.N;

		const commit = table.rows[1].cells[7];
		commit.textContent = truncate(test.commithash, 12);

		const eloall = table.rows[1].cells[3];
		eloall.textContent = formatElo(test.eloall, test.pmall);

		const eloweighted = table.rows[1].cells[4];
		eloweighted.textContent = formatElo(test.eloweighted, test.pmweighted);

		const elocentral = table.rows[1].cells[5];
		elocentral.textContent = formatElo(test.elocentral, test.pmcentral);

		const time = table.rows[1].cells[8];
		const timeheader = table.tHead.rows[0].cells[8];
		if (test.donetime) {
			timeheader.textContent = 'Done Timestamp';
			time.textContent = formatDate(test.donetime);
		}
		else if (test.starttime) {
			timeheader.textContent = 'Start Timestamp';
			time.textContent = formatDate(test.starttime);
		}
		else {
			timeheader.textContent = 'Queue Timestamp';
			time.textContent = formatDate(test.queuetime);
		}

		const button = document.getElementById('actionbutton');
		if (test.status == 'cancelled')
			button.textContent = 'Resume';
		else
			button.textContent = 'Cancel';

		for (const [key, param] of Object.entries(test.spsa)) {
			for (const row of paramtable.rows) {
				if (row.dataset.name !== key)
					continue;

				row.dataset.name = key;

				const mean = row.cells[1];
				if (param.mean != null) {
					mean.textContent = param.mean.toFixed(3);
				}

				const maximum = row.cells[2];
				if (param.maximum != null) {
					maximum.textContent = param.maximum.toFixed(3);
				}
			}
		}

		spsa = test.spsa;
		spsahistory = test.spsahistory;

		drawImage()
	});
}
