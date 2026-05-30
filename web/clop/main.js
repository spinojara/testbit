async function getData(id) {
	const response = await fetch(`http://192.168.1.214:3333/clop/${id}`);
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

	var i = 1;
	var length = spsahistory.length + 1;
	for (const point of spsahistory) {
		var weight = point._weight ** 2;
		var phase = i / length;

		if (axisX !== '')
			x = Math.round((canvas.width - 1) * (point[axisX] - minX) / (maxX - minX));
		else
			x = Math.round((canvas.width - 1) * phase);
		if (axisY !== '')
			y = Math.round((canvas.height - 1) * (maxY - point[axisY]) / (maxY - minY));
		else
			y = Math.round((canvas.height - 1) * (1.0 - phase));

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
	test = data.test;
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
	desc = row.insertCell();
	desc.textContent = truncate(test.description, 33);
	stat = row.insertCell();
	stat.textContent = test.status;

	N = row.insertCell();
	N.textContent = test.N;
	N.style.textAlign = 'right';

	eloall = row.insertCell();
	eloall.textContent = formatElo(test.eloall, test.pmall);
	eloall.style.textAlign = 'right';

	eloweighted = row.insertCell();
	eloweighted.textContent = formatElo(test.eloweighted, test.pmweighted);
	eloweighted.style.textAlign = 'right';

	elocentral = row.insertCell();
	elocentral.textContent = formatElo(test.elocentral, test.pmcentral);
	elocentral.style.textAlign = 'right';

	tc = row.insertCell();
	tc.textContent = test.tc;
	tc.style.textAlign = 'center';

	commit = row.insertCell();
	commit.textContent = truncate(test.commithash, 12);
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

function continuerequest() {
	const passwordprompt = document.getElementById('passwordprompt');
	passwordprompt.classList = 'hidden';
	const passwordinput = document.getElementById('passwordinput');
	const password = passwordinput.value;
	passwordinput.value = '';
	const credentials = btoa(':' + password);

	const button = document.getElementById('actionbutton');
	var endpoint = 'http://192.168.1.214:3333/test/';
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
