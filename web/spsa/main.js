async function getData(id) {
	const response = await fetch(`https://jalagaoi.se/testbit/spsa/${id}`);
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

function formatDate(unixepoch) {
	if (!unixepoch) {
		return ''
	}
	const date = new Date(unixepoch * 1000);
	const year = String(date.getFullYear());
	const month = String(date.getMonth() + 1);
	const day = String(date.getDate());
	const hour = String(date.getHours());
	const minute = String(date.getMinutes());
	const second = String(date.getSeconds());
	return `${year}-${month.padStart(2, '0')}-${day.padStart(2, '0')} ${hour.padStart(2, '0')}:${minute.padStart(2, '0')}:${second.padStart(2, '0')}`
}

function formatPatch(patch) {
	parsedPatch = '';
	open = false;
	openatat = false;
	betweengitdiffandatat = false;
	readingBinary = false;

	for (let i = 0; i < patch.length; i++) {
		var c = patch[i];

		if (i == 0 && patch.substring(0, 18) == 'Legacy NNUE patch\n') {
			parsedPatch += '<span class=\'magenta\'>';
			open = true;
		}

		if (!betweengitdiffandatat && ((i == 0 && patch.substring(0, 11) == 'diff --git ') || patch.substring(i, i + 12) == '\ndiff --git ')) {
			if (open)
				parsedPatch += '</span>';
			parsedPatch += '<span class=\'white\'>';
			open = true;
			betweengitdiffandatat = true;
			readingBinary = false;
		}

		if (!betweengitdiffandatat && c == '\n') {
			if (open)
				parsedPatch += '</span>';
			open = false;
		}

		if (patch.substring(i - 18, i) == '\nGIT binary patch\n') {
			readingBinary = true;
			betweengitdiffandatat = false;
		}

		if (patch.substring(i, i + 4) == '\n@@ ') {
			if (open)
				parsedPatch += '</span>';
			parsedPatch += '<span class=\'bold cyan\'>';
			open = true;
			openatat = true;
			betweengitdiffandatat = false;
		}

		if (i > 3 && patch.substring(i - 3, i + 1) == ' @@ ') {
			if (openatat) {
				parsedPatch += '</span>';
				openatat = false;
				open = false;
			}
		}

		if (!betweengitdiffandatat && patch.substring(i, i + 2) == '\n+') {
			if (open)
				parsedPatch += '</span>';
			parsedPatch += '<span class=\'bold green\'>';
			open = true;
		}
		else if (!betweengitdiffandatat && patch.substring(i, i + 2) == '\n-') {
			if (open)
				parsedPatch += '</span>';
			parsedPatch += '<span class=\'bold red\'>';
			open = true;
		}

		if (readingBinary)
			continue;

		switch (c) {
		case '<':
			c = '&lt;';
			break;
		case '>':
			c = '&gt;';
			break;
		case '&':
			c = '&amp;';
			break;
		case '"':
			c = '&quot;';
			break;
		case '\'':
			c = '&#039;';
			break;
		default:
			break;
		}
		parsedPatch += c;
	}

	if (open)
		parsedPatch += '</span>';

	return parsedPatch;
}

function parseEscapeCodes(text) {
	if (!text)
		return null;
	var parsedText = '';
	var readingEscape = false;
	var escapeCode;
	var open = 0;

	for (let i = 0; i < text.length; i++) {
		var c = text[i];
		if (c == '\u001b') {
			readingEscape = true;
			escapeCode = '';
			continue;
		}

		if (!readingEscape) {
			switch (c) {
			case '<':
				c = '&lt;';
				break;
			case '>':
				c = '&gt;';
				break;
			case '&':
				c = '&amp;';
				break;
			case '"':
				c = '&quot;';
				break;
			case '\'':
				c = '&#039;';
				break;
			default:
				break;
			}
			parsedText += c;
			continue;
		}

		switch (c) {
		case '[':
			break;
		case 'm':
			readingEscape = false;
		case ';':
			switch (escapeCode) {
			case '0':
				while (open > 0) {
					parsedText += '</span>';
					open--;
				}
				break;
			case '1':
				parsedText += '<span class=\'bold\'>';
				open += 1;
				break;
			case '3':
				parsedText += '<span class=\'italic\'>';
				open += 1;
				break;
			case '4':
				parsedText += '<span class=\'underline\'>';
				open += 1;
				break;
			case '90':
				parsedText += '<span class=\'bold\'>';
				open += 1;
			case '30':
				parsedText += '<span class=\'black\'>';
				open += 1;
				break;
			case '91':
				parsedText += '<span class=\'bold\'>';
				open += 1;
			case '31':
				parsedText += '<span class=\'red\'>';
				open += 1;
				break;
			case '92':
				parsedText += '<span class=\'bold\'>';
				open += 1;
			case '32':
				parsedText += '<span class=\'green\'>';
				open += 1;
				break;
			case '93':
				parsedText += '<span class=\'bold\'>';
				open += 1;
			case '33':
				parsedText += '<span class=\'yellow\'>';
				open += 1;
				break;
			case '94':
				parsedText += '<span class=\'bold\'>';
				open += 1;
			case '34':
				parsedText += '<span class=\'blue\'>';
				open += 1;
				break;
			case '95':
				parsedText += '<span class=\'bold\'>';
				open += 1;
			case '35':
				parsedText += '<span class=\'magenta\'>';
				open += 1;
				break;
			case '96':
				parsedText += '<span class=\'bold\'>';
				open += 1;
			case '36':
				parsedText += '<span class=\'cyan\'>';
				open += 1;
				break;
			case '97':
				parsedText += '<span class=\'bold\'>';
				open += 1;
			case '37':
				parsedText += '<span class=\'white\'>';
				open += 1;
				break;
			default:
				console.log('Unknown escape code: \\' + escapeCode);
			}
			escapeCode = '';
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			escapeCode += c;
			break;
		default:
			console.log('Illegal character in escape code');
		}
	}
	while (open > 0) {
		parsedText += '</span>';
		open--;
	}
	return parsedText;
}

function getid() {
	const params = new URLSearchParams(window.location.search);
	const id = params.get('id') || null;
	return id;
}

id = getid();

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

	axisX = '';
	axisY = '';
	minX = 0.0;
	maxX = 1.0;
	minY = 0.0;
	maxY = 1.0;
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

	var i = length === 1 ? 1 : 0;
	const lastElement = {};
	for (const [key, param] of Object.entries(spsa)) {
		lastElement[key] = param.theta;
	}

	const merged = [...spsahistory, lastElement];
	length = Math.max(merged.length - 1, 1);
	for (const point of merged) {
		const phase = (i / length);

		if (axisX !== '')
			x = Math.round((canvas.width - 1) * (point[axisX] - minX) / (maxX - minX));
		else
			x = Math.round((canvas.width - 1) * phase);
		if (axisY !== '')
			y = Math.round((canvas.height - 1) * (maxY - point[axisY]) / (maxY - minY));
		else
			y = Math.round((canvas.height - 1) * (1.0 - phase));

		var r = 255;
		var g = 0;
		var b = 255;
		if (i !== merged.length - 1) {
			r = 255 * (1.0 - phase);
			g = 255 * phase;
			b = 0;
		}

		ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;
		ctx.fillRect(x, y, 1, 1);
		ctx.fill();

		i++;
	}

}

getData(id).then(data => {
	if (data.message != 'ok')
		redirectHome();
	test = data.test;
	const headers = ['Description', 'Status', 'N', 'Alpha', 'Gamma', 'A', 'TC', 'Commit ID'];

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
	desc = row.insertCell();
	desc.textContent = truncate(test.description, 33);
	stat = row.insertCell();
	stat.textContent = test.status;

	N = row.insertCell();
	N.textContent = test.N;
	N.style.textAlign = 'right';

	alpha = row.insertCell();
	alpha.textContent = test.alpha;
	alpha.style.textAlign = 'right';

	gamma = row.insertCell();
	gamma.textContent = test.gamma;
	gamma.style.textAlign = 'right';

	A = row.insertCell();
	A.textContent = test.A;
	A.style.textAlign = 'right';

	tc = row.insertCell();
	tc.textContent = test.tc;
	tc.style.textAlign = 'center';

	commit = row.insertCell();
	commit.textContent = truncate(test.commit, 12);
	commit.style.textAlign = 'center';

	time = row.insertCell();
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
	preerrorlog.innerHTML = parseEscapeCodes(test.errorlog);
	if (!test.errorlog)
		preerrorlog.style.display = 'none';

	const button = document.getElementById('actionbutton');
	button.onclick = promptpassword;
	if (test.status == 'cancelled')
		button.textContent = 'Resume';
	else
		button.textContent = 'Cancel';

	const paramheaders = ['Parameter', 'Value', 'Min', 'Max', 'a', 'c', 'ak', 'ck', 'Step Size']
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

		const value = row2.insertCell();
		value.textContent = param.theta.toFixed(3);
		value.style.textAlign = 'right';

		const min = row2.insertCell();
		min.textContent = param.min.toFixed(3);
		min.style.textAlign = 'right';

		const max = row2.insertCell();
		max.textContent = param.max.toFixed(3);
		max.style.textAlign = 'right';

		const a = row2.insertCell();
		a.textContent = param.a.toFixed(3);
		a.style.textAlign = 'right';

		const c = row2.insertCell();
		c.textContent = param.c.toFixed(3);
		c.style.textAlign = 'right';

		const akval = (param.a / ((test.A + test.N + 1) ** test.alpha));
		const ak = row2.insertCell();
		ak.textContent = akval.toFixed(7);
		ak.style.textAlign = 'right';
		const ckval = (param.c / ((test.A + test.N + 1) ** test.gamma));
		const ck = row2.insertCell();
		ck.textContent = ckval.toFixed(7);
		ck.style.textAlign = 'right';

		const stepsize = row2.insertCell();
		stepsize.textContent = (akval / ckval).toFixed(7);
		stepsize.style.textAlign = 'right';

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

function continuerequest() {
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
}

function wrongpasswordok() {
	const wrongpassword = document.getElementById('wrongpassword');
	wrongpassword.classList = 'hidden';
}

function cancelrequest() {
	const passwordprompt = document.getElementById('passwordprompt');
	passwordprompt.classList = 'hidden';
	const passwordinput = document.getElementById('passwordinput');
	passwordinput.value = '';
}

function updateTable() {
	if (!shouldUpdate)
		return;
	getData(id).then(data => {
		if (data.message != 'ok')
			redirectHome();
		test = data.test;
		shouldUpdate = test.donetime == null;

		const statusCell = table.rows[1].cells[1];
		statusCell.textContent = test.status;

		const NCell = table.rows[1].cells[2];
		NCell.textContent = test.N;

		const commit = table.rows[1].cells[7];
		commit.textContent = truncate(test.commit, 12);

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

				const value = row.cells[1];
				value.textContent = param.theta.toFixed(3);

				const akval = (param.a / ((test.A + test.N + 1) ** test.alpha));
				const ak = row.cells[6];
				ak.textContent = akval.toFixed(7);
				ak.style.textAlign = 'right';

				const ckval = (param.c / ((test.A + test.N + 1) ** test.gamma))
				const ck = row.cells[7];
				ck.textContent = ckval.toFixed(7);
				ck.style.textAlign = 'right';

				const stepsize = row.cells[8];
				stepsize.textContent = (akval / ckval).toFixed(7);
				stepsize.style.textAlign = 'right';
			}
		}

		spsa = test.spsa;
		spsahistory = test.spsahistory;

		drawImage()
	});
}
