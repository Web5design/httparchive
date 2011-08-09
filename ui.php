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


require_once("utils.php");


function genTitle($addl = "") {
	global $gbMobile;

	return "HTTP Archive " . 
		( $gbMobile ? "Mobile " : "" ) . 
		( $addl ? " - $addl" : "" );
}


function globalCss() {
	return <<<OUTPUT
<style>
BODY { font-family: Arial; }
.header { border-bottom: 4px groove #17598F; font-size: 3em; color: #17598F; }
.preheader { font-size: 0.8em; }
.notification { color: #C00; }
.tdnum { text-align: right; }
</style>

OUTPUT;
}


// HTML to insert first inside the HEAD element.
function headfirst() {
	// Google Analytics
	return <<<OUTPUT
<script type="text/javascript">
// Google Analytics
var _gaq = _gaq || [];
_gaq.push(['_setAccount', 'UA-22381566-1']);
_gaq.push(['_setDomainName', '.httparchive.org']);
_gaq.push(['_trackPageview']);
_gaq.push(['_trackPageLoadTime']);
(function() {
	var ga = document.createElement('script'); ga.type = 'text/javascript'; ga.async = true;
	ga.src = ('https:' == document.location.protocol ? 'https://ssl' : 'http://www') + '.google-analytics.com/ga.js';
	var s = document.getElementsByTagName('script')[0]; s.parentNode.insertBefore(ga, s);
})();
</script>
<!--[if lt IE 9]>
<script>
// we use some new HTML tags - make them work in IE<9 - hrrmphf
var e = ("abbr,article,aside,audio,canvas,datalist,details,figure,footer,header,hgroup,mark,menu,meter,nav,output,progress,section,time,video").split(',');
for (var i = 0; i < e.length; i++) {
	document.createElement(e[i]);
}
</script>
<![endif]-->

OUTPUT;
}


function uiHeader($title = "HTTP Archive", $bNavlinks = true, $extraNav='') {
	global $gbMobile;

	$navlinks = "";
	if ( $bNavlinks ) {
		$navlinks = '
<nav>
	<ul>
		<li> <a href="trends.php">Trends</a>
		<li> <a href="interesting.php">Stats</a>
		<li><a href=websites.php>Websites</a>
		<li><a href=about.php>About</a></li>
    </ul>
</nav>';
	}

	$mobile = "";
	$mobitest = "";
	$version = "BETA";
	if ( $gbMobile ) {
		$mobile = "<br><span style='font-style: italic; font-size: 0.7em;'>Mobile</span>";
		$mobitest = "<div style='position: absolute; left: 780px; top: 10px;'><a href='http://www.blaze.io/mobile/' alt='Web Performance Tool'><img src='/images/archivemobitest.png' border=0></a></div>";
		$version = "ALPHA";
	}

	if ( $version ) {
		$version = "<div style='position: absolute; width: 211px; top: 20px; text-align: right;'>$version</div>";
	}

	return <<<OUTPUT
    <header>
$mobitest
$version
		<a href="index.php" id="logo" style="line-height: 0.7em;">HTTP Archive$mobile</a>
		$navlinks
	</header>

OUTPUT;
}


function uiFooter() {
	return <<<OUTPUT
<footer style="text-align: center;">
<a href="about.php#sponsors">sponsors</a>: 
<a title="Google" href="http://www.google.com/">Google</a>,
<a title="Mozilla" href="http://www.mozilla.org/firefox">Mozilla</a>,
<a title="New Relic" href="http://www.newrelic.com/">New Relic</a>,
<a title="O'Reilly Media" href="http://oreilly.com/">O&#8217;Reilly Media</a>,
<a href="http://www.etsy.com/">Etsy</a>,
<a title="Strangeloop Networks" href="http://www.strangeloopnetworks.com/">Strangeloop</a>,
and <a title="dynaTrace Software" href="http://www.dynatrace.com/">dynaTrace Software</a>
</footer>
OUTPUT;
}

?>
