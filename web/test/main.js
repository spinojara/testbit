async function getData(id) {
	const response = await fetch(`https://jalagaoi.se:2718/test/${id}`);
	const json = await response.json();
	return await json;
}

function redirectHome() {
	window.location.href = '/';
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

	for (let i = 0; i < patch.length; i++) {
		const c = patch[i];
		if (c == '\n' && i < patch.length - 1) {
			if (open)
				parsedPatch += '</span>';
			open = false;
			if (patch[i + 1] == '+') {
				parsedPatch += '<span class=\'green bold\'>'
				open = true;
			}
			if (patch[i + 1] == '-') {
				parsedPatch += '<span class=\'red bold\'>'
				open = true;
			}
		}

		parsedPatch += c;
	}

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
		const c = text[i];
		if (c == '\u001b') {
			readingEscape = true;
			escapeCode = '';
			continue;
		}

		if (!readingEscape) {
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

const params = new URLSearchParams(window.location.search);
const id = params.get('id') || null;

if (!id)
	redirectHome();

const table = document.getElementById('testtable');
const buttondiv = document.getElementById('buttondiv');
const prepatch = document.getElementById('patch')
const preerrorlog = document.getElementById('errorlog')
const thead = table.createTHead();
const headerRow = thead.insertRow();

getData(id).then(data => {
	if (data.message != 'ok')
		redirectHome();
	test = data.test;
	const headers = ['Description', 'Type', 'Status', 'Elo', 'Trinomial', 'Pentanomial'];

	if (test.type == 'sprt')
		headers.push(...['LLR', '(A, B)', '\u03B1', '\u03B2', 'Elo 0', 'Elo 1']);
	else
		headers.push('Elo Error');

	headers.push(...['TC', 'Commit ID'])

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
	desc.textContent = test.description;
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
	if (test.type == 'sprt') {
		llr = row.insertCell();
		if (test.llr)
			llr.textContent = test.llr.toFixed(3);

		AB = row.insertCell();
		AB.textContent = "(" + Math.log(test.beta / (1.0 - test.alpha)).toFixed(3) + ", " + Math.log((1.0 - test.beta) / test.alpha).toFixed(3) + ")";
		AB.style.textAlign = 'center';

		alpha = row.insertCell();
		alpha.textContent = test.alpha;
		alpha.style.textAlign = 'right';

		beta = row.insertCell();
		beta.textContent = test.beta;
		beta.style.textAlign = 'right';

		elo0 = row.insertCell();
		elo0.textContent = test.elo0.toFixed(3);
		elo0.style.textAlign = 'right';

		elo1 = row.insertCell();
		elo1.textContent = test.elo1.toFixed(3);
		elo1.style.textAlign = 'right';
	}
	else {
		eloe = row.insertCell();
		eloe.textContent = test.eloe.toFixed(3);
		eloe.style.textAlign = 'right';
	}

	tc = row.insertCell();
	tc.textContent = test.tc;
	tc.style.textAlign = 'center';

	commit = row.insertCell();
	commit.textContent = test.commit;
	commit.style.textAlign = 'center';

	time = row.insertCell();
	if (time.donetime) {
		time.textContent = formatDate(test.donetime);
	}
	else if (test.starttime) {
		time.textContent = formatDate(test.starttime);
	}
	else if (test.queuetime) {
		time.textContent = formatDate(test.queuetime);
	}
	time.style.textAlign = 'center';

	prepatch.innerHTML = formatPatch(test.patch);
	preerrorlog.innerHTML = parseEscapeCodes(test.errorlog);
	if (!test.errorlog)
		preerrorlog.style.display = 'none';

	if (test.status == 'cancelled') {
		const button = document.createElement('button');
		button.textContent = 'Resume';
		buttondiv.appendChild(button);
	}
	else if (test.status == 'running' || test.status == 'building' || test.status == 'queued') {
		const button = document.createElement('button');
		button.textContent = 'Cancel';
		buttondiv.appendChild(button);
	}
	else {
		const button = document.createElement('button');
		button.textContent = 'Requeue';
		buttondiv.appendChild(button);
	}
});
