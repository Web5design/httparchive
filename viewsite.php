<?php 
/*
Copyright 2010 Google Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

require_once("ui.php");
require_once("utils.php");

if ( getParam('pageid') ) {
	$gPageid = getParam('pageid');
	$query = "select * from $gPagesTable where pageid='$gPageid';";
}
else if ( ! $gPageid && getParam('u') && getParam('l') ) {
	$url = getParam('u');
	$gLabel = getParam('l');
	$query = "select * from $gPagesTable where url='$url' and label='$gLabel';";
}
else {
	// should never reach here
	header('Location: websites.php');
	return;
}

// TODO - better error handling starting here!
// Changed to select * to allow summary paragraph
$row = doRowQuery($query);

$gPageid = $row['pageid'];
$gArchive = $row['archive'];
$gLabel = $row['label'];
$gTitle = "View Site";
$harfile = $row['harfile'];
$url = $row['url'];
$wptid = $row['wptid'];
$wptrun = $row['wptrun'];
$onLoad = $row['onLoad'];
$renderStart = $row['renderStart'];

$server = 'http://httparchive.webpagetest.org/';  // also in bulktest/settings.inc
$harfileWptUrl = "{$server}export.php?test=$wptid&run=$wptrun&cached=0";

?>
<!doctype html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<title>HTTP Archive - <?php echo htmlentities($url) ?></title>

<?php echo headfirst() ?>
<link type="text/css" rel="stylesheet" href="style.css" />
<link rel="stylesheet" href="har.css" type="text/css">
</head>

<body class=viewsite id=top>
<?php echo uiHeader($gTitle); ?>

	<h1><?php echo str_replace('>http://', '><span class=protocol>http://</span>', siteLink($url)) ?></h1>
	
	<p class=summary>took <?php echo round(($onLoad / 1000), 1) ?> seconds to load <?php echo round(($row['bytesTotal']/1024)) ?>kB of data over <?php echo $row['reqTotal'] ?> requests.</p>

	<ul class=quicklinks>
		<li><a href="#top">Top of page</a></li>
		<li><a href="#filmstrip">Filmstrip</a></li>
		<li><a href="#waterfall">Waterfall</a></li>
		<li><a href="#pagespeed">Page Speed</a></li>
		<li><a href="#requests">Requests</a></li>
		<li><a href="#trends">Trends</a></li>
		<li><a href="#downloads">Downloads</a></li>
	</ul>
	
	<?php echo selectSiteLabel($url, $gLabel) ?>


<h2 id=filmstrip>Filmstrip, Video</h2>

<?php 
// Build a table that has empty cells to be filled in later.
if ( ! onBlacklist($url) ) {
	echo <<<OUTPUT
<section id="videoContainer">
<div id="videoDiv">
</div>
</section>


<script>
function showInterval(ms) {
	var table = document.getElementById('video');
	var aThs = table.getElementsByTagName('th');
	var aTds = table.getElementsByTagName('td');
	var len = aTds.length;
	var prevSrc;
	for ( var i = 0; i < len; i++ ) {
		var t = aTds[i].id.substring(2);
		var sDisplay = "none";
		var sBorder = "0px";
		var img = aTds[i].getElementsByTagName('img')[0];
		if ( 0 === ( t % ms ) || i === len-1 ) {
			sDisplay = "table-cell";
			if ( !img.src && img.id ) {
				img.src = img.id;
			}
			if ( prevSrc != img.src ) {
				prevSrc = img.src;
				if ( 0 < t ) {
					sBorder = "3px solid #000";
				}
			}
		}
		aTds[i].style.display = sDisplay;
		aThs[i].style.display = sDisplay;
		img.style.border = sBorder;
	}

	var sel = document.getElementById('interval');
	for ( var i = 0; i < sel.options.length; i++ ) {
		var option = sel.options[i];
		if ( option.value == ms ) {
			option.selected = true;
			break;
		}
	}
}
</script>

<form>
<label for=interval>Show screenshots every:</label>
<select id=interval onchange='showInterval(this.options[this.selectedIndex].value)'>
<option value=100>0.1 seconds</option>
<option value=500>0.5 seconds</option>
<option value=1000>1 second</option>
<option value=5000>5 seconds</option>
</select>
</form>

<script type="text/javascript">
// Load this async since it does an RPC for the filmstrip XML.
var filmstripjs = document.createElement('script');
filmstripjs.src = "filmstrip.js?pageid=$gPageid";
document.getElementsByTagName('head')[0].appendChild(filmstripjs);
</script>

OUTPUT;
}
?>

<ul class=horizlist>
  <li> <a href="http://httparchive.webpagetest.org/video/compare.php?tests=<?php echo $wptid ?>-r:<?php echo $wptrun ?>-c:0">WPT filmstrip</a>
  <li> <a href="http://httparchive.webpagetest.org/video/create.php?tests=<?php echo $wptid ?>-r:<?php echo $wptrun ?>-c:0&id=<?php echo $wptid ?>.<?php echo $wptrun ?>.0">watch video</a>
</ul>


<h2 id=waterfall>HTTP Waterfall</h2>

<div id=harviewer>
<div id=pageliststeve></div>
<div class=tabDOMBody id=tabDOMBody></div>
</div>


<script src='schema.js'></script>
<script src='har.js'></script>
<script src='harviewer.js?u=<?php echo urlencode($harfileWptUrl) ?>'></script>

<script>
function initHAR() {
	var _3cb = document.getElementById('pageliststeve');
	var _3cc = HARjson;
	if ( _3cc ) {
		// in IE there's a race condition
		if ( "undefined" === typeof(HAR.Model) ) {
			setTimeout(initHAR, 1000);
			return;
		}

		HAR.Model.appendData(_3cc);
		HAR.Tab.Preview.append(_3cc, _3cb);
		var _3ce = document.getElementById("tabDOMBody");
		_3ce.updated = false;
	}
};

initHAR();
</script>




<h2 id=pagespeed>Page Speed</h2>

<style>
#pagespeedreport UL { max-width: 100%; }
</style>
<div id="pagespeedreport" style="margin-top: 10px; font-size: 0.9em;"></div>

<script type="text/javascript" src="http://ajax.googleapis.com/ajax/libs/jquery/1.4.2/jquery.min.js"></script>
<script type="text/javascript" src="http://httparchive.webpagetest.org/widgets/pagespeed/tree?test=<?php echo $wptid ?>&div=pagespeedreport"></script>





<h2 id=requests>Requests</h2>
<a href="download.php?p=<?php echo $gPageid ?>&format=csv">download CSV</a>

<table id=stats class=tablesort border=0 cellpadding=0 cellspacing=0>
	<tr>
<th class="sortnum">req#</th> 
<th>URL</th> 
<th>mime type</th>
<th>method</th>
<th class=sortnum>status</th>
<th class="sortnum">time</th> 
<th class=sortnum>response<br>Size</th>
<th class=sortnum>request<br>Cookie Len</th>
<th class=sortnum>response<br>Cookie Len</th>
<th>request<br>Http&nbsp;Ver</th>
<th>response<br>Http&nbsp;Ver</th>
<th>request Accept</th>
<th>request Accept-Charset</th>
<th>request Accept-Encoding</th>
<th>request Accept-Language</th>
<th>request Connection</th>
<th>request Host</th>
<th>request Referer</th>

<th>response<br>Accept-Ranges</th>
<th>response<br>Age</th>
<th>response<br>Cache-Control</th>
<th>response<br>Connection</th>
<th>response<br>Content-Encoding</th>
<th>response<br>Content-Language</th>
<th>response<br>Content-Length</th>
<th>response<br>Content-Location</th>
<th>response<br>Content-Type</th>
<th>response<br>Date</th>
<th>response<br>Etag</th>
<th>response<br>Expires</th>
<th>response<br>Keep-Alive</th>
<th>response<br>Last-Modified</th>
<th>response<br>Location</th>
<th>response<br>Pragma</th>
<th>response<br>Server</th>
<th>response<br>Transfer-Encoding</th>
<th>response<br>Vary</th>
<th>response<br>Via</th>
<th>response<br>X-Powered-By</th>
</tr>

<?php
// MySQL Table
$sRows = "";
$iRow = 0;
$gFirstStart = 0;

$query = "select * from $gRequestsTable where pageid = '$gPageid';";
$result = doQuery($query);
if ( $result ) {
	while ($row = mysql_fetch_assoc($result)) {
		if ( !$gFirstStart ) {
			$gFirstStart = intval($row['startedDateTime']);
		}
		$iRow++;
		$sRow = "<tr" . ( $iRow % 2 == 0 ? " class=odd" : "" ) . ">";
		$sRow .= "<td class=tdnum>$iRow</td> ";
		$sRow .= "<td class=nobr style='font-size: 0.9em;'><a href='" . $row['url'] . "'>" . shortenUrl($row['url']) . "</a></td> ";
		$sRow .= tdStat($row, "mimeType", "", "nobr");
		$sRow .= tdStat($row, "method", "", "");
		$sRow .= tdStat($row, "status");
		$sRow .= tdStat($row, "time");
		$sRow .= tdStat($row, "respSize", "kB");
		$sRow .= tdStat($row, "reqCookieLen", "b");
		$sRow .= tdStat($row, "respCookieLen", "b");
		$sRow .= tdStat($row, "reqHttpVersion", "", "");
		$sRow .= tdStat($row, "respHttpVersion", "", "");
		$sRow .= tdStat($row, "req_accept", "snip", "nobr");
		$sRow .= tdStat($row, "req_accept_charset", "", "");
		$sRow .= tdStat($row, "req_accept_encoding", "", "nobr");
		$sRow .= tdStat($row, "req_accept_language", "", "");
		$sRow .= tdStat($row, "req_connection", "", "");
		$sRow .= tdStat($row, "req_host", "", "");
		$sRow .= tdStat($row, "req_referer", "url", "");
		$sRow .= tdStat($row, "resp_accept_ranges", "", "");
		$sRow .= tdStat($row, "resp_age", "", "");
		$sRow .= tdStat($row, "resp_cache_control", "", "");
		$sRow .= tdStat($row, "resp_connection", "", "");
		$sRow .= tdStat($row, "resp_content_encoding", "", "");
		$sRow .= tdStat($row, "resp_content_language", "", "");
		$sRow .= tdStat($row, "resp_content_length", "", "");
		$sRow .= tdStat($row, "resp_content_location", "url", "");
		$sRow .= tdStat($row, "resp_content_type", "", "");
		$sRow .= tdStat($row, "resp_date", "", "nobr");
		$sRow .= tdStat($row, "resp_etag", "", "");
		$sRow .= tdStat($row, "resp_expires", "", "nobr");
		$sRow .= tdStat($row, "resp_keep_alive", "", "");
		$sRow .= tdStat($row, "resp_last_modified", "", "nobr");
		$sRow .= tdStat($row, "resp_location", "url", "");
		$sRow .= tdStat($row, "resp_pragma", "", "");
		$sRow .= tdStat($row, "resp_server", "", "");
		$sRow .= tdStat($row, "resp_transfer_encoding", "", "");
		$sRow .= tdStat($row, "resp_vary", "", "");
		$sRow .= tdStat($row, "resp_via", "", "");
		$sRow .= tdStat($row, "resp_x_powered_by", "", "");

		$sRows .= $sRow;
	}
	mysql_free_result($result);
}
echo $sRows;
?>
</table>

<script type="text/javascript">
var tsjs = document.createElement('script');
tsjs.src = "tablesort.js";
tsjs.onload = function() { TS.init(); };
tsjs.onreadystatechange = function() { if ( tsjs.readyState == 'complete' || tsjs.readyState == 'loaded' ) { TS.init(); } };
document.getElementsByTagName('head')[0].appendChild(tsjs);
</script>



<h2 id=trends>Trends</h2>

<?php
$gSet = "url";
$gUrl = $url;
require_once('trends.inc');
?>


<h2 id=downloads>Downloads</h2>

<div style='margin-top: 4px; font-size: 0.8em;'>
<a href='<?php echo $harfileWptUrl ?>'>download HAR file</a>
</div>


<?php echo uiFooter() ?>

</body>

</html>

<?php
function tdStat($row, $field, $suffix = "", $class = "tdnum") {
	global $gFirstStart;

	$value = $row[$field];
	$snipmax = 12;

	if ( "kB" === $suffix ) {
		if ( 0 == $value ) {
			$value = "&nbsp;";
			$suffix = "";
		}
		else {
			$value = formatSize($value);
			if ( 0 == $value ) {
				$value = 1;  // don't round down to zero
			}
		}
	}
	else if ( "b" === $suffix ) {
		if ( 0 == $value ) {
			$value = "&nbsp;";
			$suffix = "";
		}
	}
	else if ( "snip" === $suffix ) {
		$suffix = "";
		if ( strlen($value) > ($snipmax+5) ) {
			$value = "<a href='javascript:' title='$value'>" . substr($value, 0, $snipmax) . "</a>";
		}
	}
	else if ( "url" === $suffix ) {
		$suffix = "";
		$url = $value;
		$iLastSlash = strrpos($url, "/");
		$sFilename = substr($url, $iLastSlash, $snipmax);
		$value = "<a href='$url'>$sFilename</a>";
	}
	else if ( "start" == $suffix ) {
		$suffix = "";
		$value = intval($value) - $gFirstStart;
	}

	if ( $class ) {
		$class = " class=$class";
	}
	
	return ( $suffix ? "<td$class>$value&nbsp;$suffix</td>" : "<td$class>$value</td>" );
}
?>
