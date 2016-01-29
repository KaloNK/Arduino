var LoggedIn = false;

function ShowMessages()
{
	var STR = document.location.search.split("MSG=")[1];
	if (typeof STR === "string" && STR.length) STR.split("&")[0];
	if (typeof STR === "string" && STR.length) {
		STR = decodeURI(STR);
		alert(STR);
		if ( STR == "Login successful" ) {
			document.execCommand("ClearAuthenticationCache", "false");
			if (window.crypto && typeof window.crypto.logout === "function") window.crypto.logout();
			else {
				var request = new XMLHttpRequest();
				request.open("GET", "/Config?Action=ClearAuth", false, "false", "false");
				request.send();
			}
		}
		window.location = "/Static/";
	}
}

function CheckLogin()
{
	var STR = document.cookie.match(new RegExp("ESPSESSIONID=([^;]+)"));
	if (!STR || !STR[0]) {
		alert("Please Login");
		window.location = "/Config?Action=Login";
	} else if (STR && STR[0] == "0") alert("Logged out");
	else if (STR && STR[0] > 0) LoggedIn = true;
}

function ToggleMenu(ItemID)
{
	var el = document.getElementById('MenuItem_'+ItemID);
	if ( typeof(el) != "undefined" && el && typeof(el.style) != "undefined" && typeof(el.style.display) != "undefined" )
	{
		if ( el.style.display != 'none' ) el.style.display = 'none';
		else el.style.display = '';
	}
	return false;
}

function HideAllMenuItems()
{
	var spans = document.getElementsByTagName('span');
	for (var i=0;i<spans.length;i++) {
		if ( spans[i].getAttribute("class") === "MenuItem" ) {
			if ( spans[i].style.display != 'none' ) spans[i].style.display = 'none';
		}
	}
}

function MultiplyIt(name, max_count)
{
	var GetIt = document.getElementById(name+"_GetIt");
	var PlaceIt = document.getElementById(name+"_PlaceIt");
	if ( typeof(GetIt) == "undefined" || !GetIt || typeof(GetIt.innerHTML) == "undefined" ) {
		alert("Can't find where to 'GetIt'");
		return;
	}
	if ( typeof(PlaceIt) == "undefined" || !PlaceIt || typeof(PlaceIt.innerHTML) == "undefined" ) {
		alert("Can't find where to 'PlaceIt'");
		return;
	}
	if ( typeof(window.MultiplyItCount) == "undefined" ) {
		var tmp = [];
		tmp[name] = 0;
		window.MultiplyItCount = tmp
	}
	if ( ++window.MultiplyItCount[name] > max_count ) {
		alert("Max number of copies reached");
		return;
	}

	PlaceIt.innerHTML += GetIt.innerHTML.replace(/NNN/g, window.MultiplyItCount[name]);
}

function ValidPassphrase(PWD)
{
	if ( PWD.value.length < 8 ) {
		alert("Passphrase too short - 8 chars min");
		PWD.value = PWD.defaultValue;
		return false;
	}
	if ( PWD.value.length > 64 ) {
		alert("Passphrase too long - 64 chars max");
		PWD.value = PWD.defaultValue;
		return false;
	}
	return true;
}

function ValidIP (TheIP)
{
	var IPvalue = TheIP.value;
	if (IPvalue == "" || IPvalue == "0" || IPvalue == "0.0.0.0") return IPvalue;

	if (/^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(IPvalue)) return true;

	alert ("Incorrect IP");
	TheIP.value = TheIP.defaultValue;
	return false;
}

function AddOptionsFromArray(el, vals)
{
	if (typeof(el) != "object" || typeof(el.options) != "object") {
		alert("Can't find the select object");
		return;
	}
	if ( vals.isArray) {
		alert("Missing values array");
		return;
	}
	for (var i=0; i<vals.length; i++) {
		el.options[el.options.length] = new Option(vals[i], vals[i]);
	}
}

function AddOptionsFromObj(el, vals)
{
	if (typeof(el) != "object" || typeof(el.options) != "object") {
		alert("Can't find the select object");
		return;
	}
	if (typeof(vals) != "object") {
		alert("Missing values object");
		return;
	}
	for (var key in vals) {
		if (vals.hasOwnProperty(key)) {
			el.options[el.options.length] = new Option(key, vals[key]);
		}
	}
}

function AddMonthOptions(el)
{
	var Months = { "Jan" : 1, "Feb" : 2, "Mar" : 3, "Apr" : 4, "May" : 5, "Jun" : 6, "Jul" : 7, "Aug" : 8, "Sep" : 9, "Oct" : 10, "Nov" : 11, "Dec" : 12 };
	AddOptionsFromObj(el, Months);
}

function AddWeekOptions(el)
{
	var Week = { "Last" : 0, "First" : 1, "Second" : 2, "Third" : 3, "Fourth" : 4 };
	AddOptionsFromObj(el, Week);
}

function AddDayOfWeekOptions(el)
{
	var DoW = { "Sun" : 1, "Mon" : 2, "Tue" : 3, "Wed" : 4, "Thu" : 5, "Fri" : 6, "Sat" : 7 };
	AddOptionsFromObj(el, DoW);
}

function AddHourOptions(el)
{
	var hours = [];
	for (var i=0; i<24; i++) hours[hours.length] = i;
	AddOptionsFromArray(el, hours);
}

function AddOffsetOptions(el)
{
	var Offsets = {
		"UTC−12:00, Y"	: -43200,
		"UTC−11:00, X"	: -39600,
		"UTC−10:00, W"	: -36000,
		"UTC−09:30, V†"	: -34200,
		"UTC−09:00, V"	: -32400,
		"UTC−08:00, U"	: -28800,
		"UTC−07:00, T"	: -25200,
		"UTC−06:00, S"	: -21600,
		"UTC−05:00, R"	: -18000,
		"UTC−04:30, Q†"	: -16200,
		"UTC−04:00, Q"	: -14400,
		"UTC−03:30, P†"	: -12600,
		"UTC−03:00, P"	: -10800,
		"UTC−02:00, O"	: -7200,
		"UTC−01:00, N"	: -3600,
		"UTC±00:00, Z"	: 0,
		"UTC+01:00, A"	: 3600,
		"UTC+02:00, B"	: 7200,
		"UTC+03:00, C"	: 10800,
		"UTC+03:30, C†"	: 12600,
		"UTC+04:00, D"	: 14400,
		"UTC+04:30, D†"	: 16200,
		"UTC+05:00, E"	: 18000,
		"UTC+05:30, E†"	: 19800,
		"UTC+05:45, E*"	: 20700,
		"UTC+06:00, F"	: 21600,
		"UTC+06:30, F†"	: 23400,
		"UTC+07:00, G"	: 25200,
		"UTC+08:00, H"	: 28800,
		"UTC+08:30, H†"	: 30600,
		"UTC+08:45, H*"	: 31500,
		"UTC+09:00, I"	: 32400,
		"UTC+09:30, I†"	: 34200,
		"UTC+10:00, K"	: 36000,
		"UTC+10:30, K†"	: 37800,
		"UTC+11:00, L"	: 39600,
		"UTC+12:00, M"	: 43200,
		"UTC+12:45, M*" : 45900,
		"UTC+13:00, M†" : 46800,
		"UTC+14:00, M†" : 50400
	};
	AddOptionsFromObj(el, Offsets);
}

function FillTZoptions(name)
{
	AddMonthOptions(document.getElementById(name+"_Month"));
	AddWeekOptions(document.getElementById(name+"_Week"));
	AddDayOfWeekOptions(document.getElementById(name+"_DoW"));
	AddHourOptions(document.getElementById(name+"_Hour"));
	AddOffsetOptions(document.getElementById(name+"_Offset"));
}

function PageInit()
{
	ShowMessages();
	HideAllMenuItems();
}

function AuthPageInit()
{
	ShowMessages();
	CheckLogin();
	HideAllMenuItems();
}
