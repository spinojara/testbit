export function formatPatch(patch) {
	var parsedPatch = '';
	var open = false;
	var openatat = false;
	var betweengitdiffandatat = false;
	var readingBinary = false;

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

export function formatDate(unixepoch) {
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

export function parseEscapeCodes(text) {
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

export function formatElo(elo, pm) {
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

