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

require_once("utils.inc");
require_once("settings.inc");
require_once("crawls.inc");

// default table names
$gPagesTable = "pages";
$gRequestsTable = "requests";
$gStatusTable = "status";
$gUrlsTable = "urls";
$gUrlsChangeTable = "urlschange";
$gStatsTable = "stats";
$gCrawlsTable = "crawls";
$gSettingsTable = "settings"; // for storing misc settings persistently

// Desktop tables
$gPagesTableDesktop = $gPagesTable;
$gRequestsTableDesktop = $gRequestsTable;
$gStatusTableDesktop = $gStatusTable;
$gUrlsTableDesktop = "urls";
$gStatsTableDesktop = $gStatsTable;
$gUrlsChangeTableDesktop = $gUrlsChangeTable;

// Mobile tables
$gPagesTableMobile = $gPagesTable . "mobile";
$gRequestsTableMobile = $gRequestsTable . "mobile";
$gUrlsTableMobile = "urls";
$gStatusTableMobile = $gStatusTable . "mobile";
$gStatsTableMobile = $gStatsTable;  // share the data table - a first step toward a single DB

// Dev tables
$gPagesTableDev = $gPagesTable . "dev";
$gRequestsTableDev = $gRequestsTable . "dev";
$gStatusTableDev = $gStatusTable . "dev";
$gStatsTableDev = $gStatsTable . "dev";
$gUrlsTableDev = $gUrlsTable . "dev";
$gUrlsChangeTableDev = $gUrlsChangeTable; // . "dev";

// 
// HERE'S WHERE WE CHANGE THE DEFAULT TABLE NAMES 
// DEPENDING ON WHETHER WE'RE DEV OR MOBILE
// 
if ( $gbDev ) {
	// Use a dev version of the database tables if "dev/" is in the path.
	$gPagesTable = $gPagesTableDev;
	$gRequestsTable = $gRequestsTableDev;
	$gStatusTable = $gStatusTableDev;
	$gStatsTable = $gStatsTableDev;
	$gUrlsTable = $gUrlsTableDev;
	$gUrlsChangeTable = $gUrlsChangeTableDev;
}
else if ( $gbMobile ) {
	// Use a mobile version of the database tables if "mobile" is in the path.
	$gPagesTable = $gPagesTableMobile;
	$gRequestsTable = $gRequestsTableMobile;
	$gStatusTable = $gStatusTableMobile;
	$gStatsTable = $gStatsTableMobile;
}



// We restrict the range of queries by adding a date range.
// Sometimes the date range is affected by querystring params.
// We also avoid old, undesired data with this.
function dateRange($bDefault = false) {
	global $gbMobile;

	// Here's the scoop: We use $dateRange as a nice global throughout most of our queries to limit
	// the scope of data. BUT...this is a problem when we want to, for example, present the user with 
	// a list of labels to choose from - we don't want that list of labels to be limited. 
	// So in some places we use $dateRangeDefault to get the WHOLE range of data.
	$dateRangeDefault = ( isPrivateInstance() ? "pageid >= 1" : ( $gbMobile ? "pageid >= 607" : "pageid >= 10281" ) );
	if ( $bDefault ) {
		return $dateRangeDefault;
	}
	else {
		// Query ALL the date ranges but avoid some undesired data (old, small runs).
		$dateRange = $dateRangeDefault;
		if ( minLabel() ) {
			$dateRange .= " and pageid >= " . labelMinid(minLabel());
		}
		if ( maxLabel() ) {
			$dateRange .= " and pageid <= " . labelMaxid(maxLabel());
		}
		return $dateRange;
	}
}


function dateRangeCrawls($bDefault = false) {
	global $gbMobile;

	// Here's the scoop: We use $dateRange as a nice global throughout most of our queries to limit
	// the scope of data. BUT...this is a problem when we want to, for example, present the user with 
	// a list of labels to choose from - we don't want that list of labels to be limited. 
	// So in some places we use $dateRangeDefault to get the WHOLE range of data.
	$dateRangeDefault = ( isPrivateInstance() ? "minPageid >= 1" : ( $gbMobile ? "minPageid >= 607" : "minPageid >= 10281" ) );
	if ( $bDefault ) {
		return $dateRangeDefault;
	}
	else {
		// Query ALL the date ranges but avoid some undesired data (old, small runs).
		$dateRange = $dateRangeDefault;
		if ( minLabel() ) {
			$dateRange .= " and minPageid >= " . labelMinid(minLabel());
		}
		if ( maxLabel() ) {
			$dateRange .= " and maxPageid <= " . labelMaxid(maxLabel());
		}
		return $dateRange;
	}
}


function dbtable($table, $device="", $label="", $slice="") {
	if ( FALSE !== stripos($device, "iphone") ) {
		$table .= "mobile";
	}
	else if ( isDev() ) {
		$table .= "dev";
	}

	return $table;
}

// These are interim helper functions for the massive mess of db table global vars
function pagesTable($label="", $slice="", $device="") {
	return dbtable("pages", $device, $label, $slice);
}

// Nothing on the production website should need access to the requests table!!
function requestsTable($label="", $slice="", $device="") {
	return dbtable("requests", $device, $label, $slice);
}

function statsTable($label="", $slice="", $device="") {
	$table = "stats";
	if ( isDev() ) {
		$table .= "dev";
	}

	return $table;
}

function crawlsTable() {
	return "crawls";
}


// return a where condition to select the appropriate URL slice based on pageid
function sliceCond($label, $slice, $device, $url = "") {
	global $gaTop100, $gaTop1000, $gArchive;

	$pagesTable = pagesTable($label, $slice, $device);
	$dateRange = dateRange();
	$crawl = getCrawl($label, $gArchive, $device);
	$minid = $crawl['minPageid'];
	$maxid = $crawl['maxPageid'];

	$sliceCond = "pageid>=$minid and pageid<=$maxid"; // "All"

	if ( 0 === strpos($slice, "Top") ) {
		// eg, "Top100" or "Top1000"
		if ( 1322727953 <= $crawl['startedDateTime'] ) {
			// The first crawl with rank available is "Dec 1 2011" (startedDateTime = 1322727953)
			$sliceCond .= " and rank > 0 and rank <= " . substr($slice, 3);
		}
		else {
			// Crawls that occurred BEFORE we started saving rank.
			if ( "Top100" === $slice || "Top1000" === $slice ) {
				// We'll use some old global variables for Top100 and Top1000.
				$urls = ( "Top100" === $slice ? $gaTop100 : $gaTop1000 );
				$query = "select pageid from $pagesTable where $sliceCond and url in ('" . implode("','", $urls) . "');";
				$result = doQuery($query);
				$aPageids = array();
				while ( $row = mysql_fetch_assoc($result) ) {
					$aPageids[] = $row['pageid'];
				}
				mysql_free_result($result);
				$sliceCond = "pageid in (" . implode(",", $aPageids) . ")";
			}
			else {
				// If you've reached this block it means we added something higher 
				// than "Top1000" (like Top10000 or Top100000) AND we're dealing 
				// with a crawl that does NOT have ranks. Choices include:
				//   1. Include all the pages. These crawls have 50K pages or less,
				//      so it might not be too far off.
				//   2. Only include the top 1000 using the global variable. This 
				//      would mean we'd compare 1000 to 10,000 or 100,000 - bad.
				//   3. Use the current rankings from the "urls" table. This would
				//      mean that the set of top URLs would constantly change.
				// I'm going with #1 - include all the pages - which means we don't 
				// have to add anything more to sliceCond.
			}
		}
	}
	else if ( "url" === $slice && isset($url) ) {
		$sliceCond .= " and url = '$url'";
	}

	return $sliceCond;
}



//
//
// MYSQL WRAPPER
//
//

// Reuse a single DB connection for the entire session.
$gDBConnection = null;

function doSimpleCommand($cmd, $link = null) {
	global $gMysqlDb;

	if ( ! $link ) {
		$link = getDBConnection();
	}

	if ( mysql_select_db($gMysqlDb) ) {
		//dprint("doSimpleCommand: $cmd");
		$result = mysql_query($cmd, $link);
		//mysql_close($link); // the findCorrelation code relies on the link not being closed
		if ( ! $result ) {
			dprint("ERROR in doSimpleCommand: '" . mysql_error() . "' for command: " . $cmd);
		}
	}
}


function doQuery($query, $link = null) {
	global $gMysqlDb;

	if ( ! $link ) {
		$link = getDBConnection();
	}

	if ( mysql_select_db($gMysqlDb) ) {
		//dprint("doQuery: $query");
		$result = mysql_query($query, $link);
		//mysql_close($link); // the findCorrelation code relies on the link not being closed
		if ( FALSE === $result ) {
			dprint("ERROR in doQuery: '" . mysql_error() . "' for query: " . $query);
		}
		return $result;
	}

	return null;
}


// return the first row
function doRowQuery($query, $link = null) {
	$row = NULL;
	$result = doQuery($query, $link);
	if ( $result ) {
		$row = mysql_fetch_assoc($result);
		mysql_free_result($result);
	}

	return $row;
}


// return the first value from the first row
function doSimpleQuery($query, $link = null) {
	$value = NULL;
	$result = doQuery($query, $link);

	if ( $result ) {
		$row = mysql_fetch_assoc($result);
		if ( $row ) {
			$aKeys = array_keys($row);
			$value = $row[$aKeys[0]];
		}
		mysql_free_result($result);
	}

	return $value;
}


// This takes an INSERT command and returns the result of last_insert_id().
// This is only useful if the INSERT command causes an AUTO_INCREMENT column to be updated.
// We have to do this because last_insert_id only works with the same connection.
// TODO - investigate only using one connection
function doLastInsertId($cmd, $link = null) {
	global $gMysqlServer, $gMysqlDb, $gMysqlUsername, $gMysqlPassword;

	if ( ! $link ) {
		$link = getDBConnection();
	}

	$last_insert_id = null;
	if ( mysql_select_db($gMysqlDb) ) {
		//dprint("doSimpleCommand: $cmd");
		$result = mysql_query($cmd, $link);
		//mysql_close($link); // the findCorrelation code relies on the link not being closed
		if ( ! $result ) {
			dprint("ERROR in doSimpleCommand: '" . mysql_error() . "' for command: " . $cmd);
		}
		else {
			$last_result = mysql_query("select last_insert_id()", $link);
			$row = mysql_fetch_array($last_result);
			$last_insert_id = $row[0];
		}
	}
	return $last_insert_id;
}


// Return a MySQL DB Connection.
// Cache this in a global variable for efficiency.
function getDBConnection() {
	global $gDBConnection, $gMysqlServer, $gMysqlUsername, $gMysqlPassword;

	// CVSNO - figure out why forking processes screws up this global variable & how to fix (mysql_ing?)
	return mysql_connect($gMysqlServer, $gMysqlUsername, $gMysqlPassword, $new_link=true);

	if ( ! $gDBConnection ) {
		$gDBConnection = mysql_connect($gMysqlServer, $gMysqlUsername, $gMysqlPassword, $new_link=true);
	}

	return $gDBConnection;
}


function tableExists($tablename) {
	return ( $tablename == doSimpleQuery("show tables like '$tablename';") );
}


/*******************************************************************************
SCHEMA CHANGES:
  This is a record of changes to the schema and how the tables were updated 
  in place.

12/1/10 - Added the "pageid" index to requestsdev. 
  This made the aggregateStats function 10x faster during import.
  mysql> create index pageid on requestsdev (pageid);

7/21/2011 - Added the "rank" column to pages table:
  mysql> alter table pagesdev add column rank int(10) unsigned after PageSpeed;

Oct 27, 2011 - Remove "harfile" column and reset unique index. Shorten columns.
  mysql> alter table pagesdev modify column archive varchar (16) not null;
  mysql> alter table pagesdev modify column label varchar (32) not null;
  mysql> drop index startedDateTime on pagesdev;
  mysql> alter table pagesdev add unique key (label, urlShort);
  mysql> alter table pagesdev drop column harfile;

Nov 3, 2011 - add "rank" column to status table
  mysql> drop table statusdev; drop table statusmobile;

Nov 16, 2011 - add perFonts column to stats table
  mysql> alter table statsdev add column perFonts int(4) unsigned after perFlash;

April 5, 2012 - add urlhash to urls table for faster lookup
  ("urlhash" is a substring of the URL's MD5 hash converted to base-10)
  mysql> alter table urls add column urlhash int(8) unsigned first;
  mysql> update urls set urlhash = conv(substring(md5(urlOrig),1,4),16,10);
  mysql> create index urlhash on urls (urlhash);

Oct 26, 2012 - add new stats columns
mysql> alter table statsdev add column renderStart int(10) unsigned after PageSpeed;
mysql> alter table statsdev add column onLoad int(10) unsigned after renderStart;
mysql> alter table statsdev add column perCompressed int(4) unsigned after perHttps;
mysql> alter table statsdev modify perHttps int(4) unsigned;

Dec 2012:
alter table pages drop column title;
  [pagesdev, pagesmobile]
alter table pages drop column urlHtmlShort;
  [pagesdev, pagesmobile]
alter table requests drop column redirectUrlShort;
  [requestsdev, requestsmobile]
alter table pages drop column urlHtml;
  [pagesdev, pagesmobile]
alter table pages add column TTFB smallint unsigned AFTER startedDateTime;
  [pagesdev, pagesmobile]
alter table pages add column reqGif smallint unsigned not null after reqImg, ADD COLUMN reqJpg smallint unsigned not null after reqGif, ADD COLUMN reqPng smallint unsigned not null after reqJpg, ADD COLUMN reqFont smallint unsigned not null after reqPng;
  [pagesdev, pagesmobile]
alter table pages add column bytesGif int(10) unsigned not null after bytesImg, ADD COLUMN bytesJpg int(10) unsigned not null after bytesGif, ADD COLUMN bytesPng int(10) unsigned not null after bytesJpg, ADD COLUMN bytesFont int(10) unsigned not null after bytesPng;
  [pagesdev, pagesmobile]
alter table pages add column maxageMore smallint unsigned not null after numDomains, add column maxage365 smallint unsigned not null after numDomains, add column maxage30 smallint unsigned not null after numDomains, add column maxage1 smallint unsigned not null after numDomains, add column maxage0 smallint unsigned not null after numDomains, add column maxageNull smallint unsigned not null after numDomains, add column numDomElements mediumint unsigned not null after numDomains, add column numCompressed smallint unsigned not null after numDomains, add column numHttps smallint unsigned not null after numDomains, add column numGlibs smallint unsigned not null after numDomains, add column numErrors smallint unsigned not null after numDomains, add column numRedirects smallint unsigned not null after numDomains, add column maxDomainReqs smallint unsigned not null after numDomains;
  [pagesdev, pagesmobile]
alter table pages add column bytesHtmlDoc mediumint unsigned not null AFTER bytesOther;
  [pagesdev, pagesmobile]
alter table pages add column fullyLoaded int(10) unsigned AFTER onLoad;
  [pagesdev, pagesmobile]

alter table pages add column cdn varchar (64) AFTER urlShort;
  [pagesdev, pagesmobile]
alter table pages add column SpeedIndex mediumint unsigned AFTER PageSpeed;
  [pagesdev, pagesmobile]
alter table pages add column visualComplete int(10) unsigned AFTER fullyLoaded;
  [pagesdev, pagesmobile]
alter table pages add column gzipTotal int unsigned not null AFTER maxageMore, add column gzipSavings int unsigned not null after gzipTotal;
  [pagesdev, pagesmobile]

alter table requests add column expAge int unsigned AFTER respCookieLen;
  [requestsdev, requestsmobile]

Jan 7, 2013 - add reqFont, bytesFont columns to stats table
alter table statsdev add column reqFont float unsigned after reqFlash;
alter table statsdev add column bytesFont int(10) unsigned after bytesFlash;

Jan 9 2013 - make expAge not null
alter table requests modify expAge int unsigned not null;

Feb 5 2013 - add new stats to stats table
alter table stats add column bytesHtmlDoc mediumint unsigned after bytesOther, add column perCdn int(4) unsigned after perGlibs, add column gzipSavings int unsigned not null after bytesHtmlDoc, add column gzipTotal int unsigned not null after bytesHtmlDoc, add column numDomElements mediumint unsigned not null after numDomains, add column maxDomainReqs smallint unsigned not null after numDomains, add column fullyLoaded int(10) unsigned after onLoad, add column visualComplete int(10) unsigned after onLoad, add column TTFB int(10) unsigned after PageSpeed, add column SpeedIndex mediumint unsigned after PageSpeed; 
  [statsdev]
*******************************************************************************/
function createTables() {
	global $gPagesTable, $gRequestsTable, $gStatusTable, $gStatsTable, $gUrlsTable, $gUrlsChangeTable, $gSettingsTable;

	if ( ! tableExists($gPagesTable) ) {
		$command = "create table $gPagesTable (" .
			"pageid int unsigned not null auto_increment" .
			", createDate int(10) unsigned not null" .
			", archive varchar (16) not null" .
			", label varchar (32) not null" .        // name of the crawl, eg, "Sep 15 2012"
			", wptid varchar (64) not null" .        // webpagetest.org id
			", wptrun int(2) unsigned not null" .    // webpagetest.org median #
			", url text" .                           // main URL fetched
			", urlShort varchar (255)" .             // shortened URL to use in index
			", cdn varchar (64)" .                   // if the base page is served by a CDN

			", startedDateTime int(10) unsigned" .   // epoch time when page was crawled 
			", TTFB smallint unsigned" .             // time to first byte of HTML doc response
			", renderStart int(10) unsigned" .       // when rendering started
			", onContentLoaded int(10) unsigned" .   // when content loaded (not available in IE)
			", onLoad int(10) unsigned" .            // window.onload
			", fullyLoaded int(10) unsigned" .       // the page is fully done (according to WPT)
			", visualComplete int(10) unsigned" .    //
			", PageSpeed int(4) unsigned" .          // PageSpeed score (0-100)
			", SpeedIndex mediumint unsigned" .      // WPT Speed Index score
			", rank int(10) unsigned" .              // worldwide rank for this URL (according to Alexa.com)

			", reqTotal int(4) unsigned not null" .  // # of resource requests, etc.
			", reqHtml int(4) unsigned not null" .
			", reqJS int(4) unsigned not null" .
			", reqCSS int(4) unsigned not null" .
			", reqImg int(4) unsigned not null" .
			", reqGif smallint unsigned not null" .
			", reqJpg smallint unsigned not null" .
			", reqPng smallint unsigned not null" .
			", reqFont smallint unsigned not null" .
			", reqFlash int(4) unsigned not null" .
			", reqJson int(4) unsigned not null" .
			", reqOther int(4) unsigned not null" .

			", bytesTotal int(10) unsigned not null" . // # of bytes TRANSFERRED (so may be bigger when uncompressed)
			", bytesHtml int(10) unsigned not null" .
			", bytesJS int(10) unsigned not null" .
			", bytesCSS int(10) unsigned not null" .
			", bytesImg int(10) unsigned not null" .
			", bytesGif int(10) unsigned not null" .
			", bytesJpg int(10) unsigned not null" .
			", bytesPng int(10) unsigned not null" .
			", bytesFont int(10) unsigned not null" .
			", bytesFlash int(10) unsigned not null" .
			", bytesJson int(10) unsigned not null" .
			", bytesOther int(10) unsigned not null" .
			", bytesHtmlDoc mediumint unsigned not null" . // size of the main HTML document

			", numDomains int(4) unsigned" .               // # of unique domains across all requests
			", maxDomainReqs smallint unsigned not null" . // # of requests on the most-used domain
			", numRedirects smallint unsigned not null" .
			", numErrors smallint unsigned not null" .
			", numGlibs smallint unsigned not null" .      // # of requests to Google Libraries API
			", numHttps smallint unsigned not null" .
			", numCompressed smallint unsigned not null" .
			", numDomElements mediumint unsigned not null" .
			", maxageNull smallint unsigned not null" .    // # of responses without a max-age value in Cache-Control
			", maxage0 smallint unsigned not null" .       // # of responses with max-age=0
			", maxage1 smallint unsigned not null" .       // # of responses with 0 < max-age <= 1 day
			", maxage30 smallint unsigned not null" .      // # of responses with 1 < max-age <= 30 days
			", maxage365 smallint unsigned not null" .     // # of responses with 30 < max-age <= 365 days
			", maxageMore smallint unsigned not null" .    // # of responses with 365 < max-age
			", gzipTotal int unsigned not null" .          // # of bytes xferred for resources that COULD have been gzipped
			", gzipSavings int unsigned not null" .        // # of bytes that could have been saved if ALL gzippable resources were gzipped

			", primary key (pageid)" .
			", unique key (label, urlShort)" .
			") ENGINE MyISAM;";
		doSimpleCommand($command);
	}

	if ( ! tableExists($gRequestsTable) ) {
		createRequestsTable($gRequestsTable);
	}

	// Create Status Table
	if ( ! tableExists($gStatusTable) ) {
		$command = "create table $gStatusTable (" .
			"statusid int unsigned not null auto_increment" .
			", url text" .
			", location varchar (32) not null" .
			", archive varchar (32) not null" .
			", label varchar (32) not null" .
			", rank int(10) unsigned" .
			", status varchar (32) not null" .
			", attempts int(2) not null" .  // how many times we've tried the URL so we can re-submit failures
			", timeOfLastChange int(10) unsigned not null" .
			", wptid varchar (64)" .
			", wptRetCode varchar (8)" .
			", medianRun int(4) unsigned" .
			", startRender int(10) unsigned" .
			", pagespeedScore int(4) unsigned" .
			", primary key (statusid)" .
			", index(statusid)" .
			") ENGINE MyISAM;";
		doSimpleCommand($command);
	}

	// Create Status Table
	$crawlsTable = crawlsTable();
	if ( ! tableExists( $crawlsTable ) ) {
		$command = "create table $crawlsTable (" .
			"crawlid int unsigned not null auto_increment" .
			", label varchar (32) not null" .
			", archive varchar (32) not null" .
			", location varchar (32) not null" . // AKA "device" - the WPT location value
			", video boolean not null" .         // true = video was recorded
			", docComplete boolean not null" .   // true = stopped at onload; false = ran later
			", fvonly boolean not null" .        // true = first view only
			", runs int(4) unsigned" .           // the number of runs for each URL (we use the median)

			", startedDateTime int(10) unsigned" .
			", finishedDateTime int(10) unsigned" .
			", timeOfLastChange int(10) unsigned not null" .
			", passes int(2) unsigned" .         // the number of passes thru the URLs - we typically do 2 (retry errors)


			", minPageid int unsigned not null" .
			", maxPageid int unsigned not null" .
			", numUrls int unsigned" .     // # of URLs submitted
			", numErrors int unsigned" .   // # of URLs that failed
			", numPages int unsigned" .    // # of URLs that resulted in valid pages
			", numRequests int unsigned" . // # of requests for all the pages

			", primary key (crawlid)" .
			", index(label, archive, location)" .
			") ENGINE MyISAM;";
		doSimpleCommand($command);
	}

	if ( ! tableExists($gStatsTable) ) {
		$command = "create table $gStatsTable (" .
			"label varchar (32) not null" .           // "Oct 1 2011"
			", slice varchar (32) not null" .         // "Top100", "Top1000", "intersection", or "All"
			", device varchar (32) not null" .        // "iphone43", "isim", or "IE8"
			", version int(6) unsigned not null" .    // the SVN version # so we can track what values to recompute
			", numurls int unsigned" .                // even "Top100" might only be 98 URLs

			// averages across ALL SITES in the slice
			", PageSpeed float unsigned" .
			", SpeedIndex mediumint unsigned" .      // WPT Speed Index score
			", TTFB int(10) unsigned" .
			", renderStart int(10) unsigned" .
			", onLoad int(10) unsigned" .
			", visualComplete int(10) unsigned" .
			", fullyLoaded int(10) unsigned" .
			", numDomains float unsigned" .
			", maxDomainReqs smallint unsigned not null" .
			", numDomElements mediumint unsigned not null" .
			", reqTotal float unsigned" .
			", reqHtml float unsigned" .
			", reqJS float unsigned" .
			", reqCSS float unsigned" .
			", reqImg float unsigned" .
			", reqGif float unsigned" .
			", reqJpg float unsigned" .
			", reqPng float unsigned" .
			", reqFlash float unsigned" .
			", reqFont float unsigned" .
			", reqJson float unsigned" .
			", reqOther float unsigned" .
			", bytesTotal int(10) unsigned" .
			", bytesHtml int(10) unsigned" .
			", bytesJS int(10) unsigned" .     // eg, average total bytes of JS downloaded per page
			", bytesCSS int(10) unsigned" .
			", bytesImg int(10) unsigned" .
			", bytesGif int(10) unsigned" .
			", bytesJpg int(10) unsigned" .
			", bytesPng int(10) unsigned" .
			", bytesFlash int(10) unsigned" .
			", bytesFont int(10) unsigned" .
			", bytesJson int(10) unsigned" .
			", bytesOther int(10) unsigned" .
			", bytesHtmlDoc mediumint unsigned" . // size of the main HTML document
			", gzipTotal int unsigned not null" .          // avg # of bytes xferred for resources that COULD have been gzipped
			", gzipSavings int unsigned not null" .        // avg # of bytes that could have been saved if ALL gzippable resources were gzipped

			// % of sites with at least one of these conditions
			", perRedirects int(4) unsigned" .
			", perErrors int(4) unsigned" .
			", perFlash int(4) unsigned" .
			", perFonts int(4) unsigned" .
			", perGlibs int(4) unsigned" .
			", perCdn int(4) unsigned" .
/*
			// % of sites that use this JS library
			", perjQuery float unsigned" .
			", perYUI float unsigned" .
			", perDojo float unsigned" .
			", perGA float unsigned" .
			", perQuantcast float unsigned" .
			", perAddThis float unsigned" .
			", perFacebook float unsigned" .
			", perGPlusOne float unsigned" .
			", perTwitter float unsigned" .
			", perShareThis float unsigned" .
*/
			// % of requests
			", perHttps int(4) unsigned" .
			", perCompressed int(4) unsigned" . 

			// % of sites with max-age in this range
			// "maxage30" === maxage > 1 day AND maxage <= 30 days
			", maxageNull int(4) unsigned" .
			", maxage0 int(4) unsigned" .
			", maxage1 int(4) unsigned" .
			", maxage30 int(4) unsigned" .
			", maxage365 int(4) unsigned" .
			", maxageMore int(4) unsigned" .

			// correlation coefficients: top 5 correlations for onload and render
			", onLoadccf1 varchar (32)" .
			", onLoadccv1 float unsigned" .
			", onLoadccf2 varchar (32)" .
			", onLoadccv2 float unsigned" .
			", onLoadccf3 varchar (32)" .
			", onLoadccv3 float unsigned" .
			", onLoadccf4 varchar (32)" .
			", onLoadccv4 float unsigned" .
			", onLoadccf5 varchar (32)" .
			", onLoadccv5 float unsigned" .
			", renderStartccf1 varchar (32)" .
			", renderStartccv1 float unsigned" .
			", renderStartccf2 varchar (32)" .
			", renderStartccv2 float unsigned" .
			", renderStartccf3 varchar (32)" .
			", renderStartccv3 float unsigned" .
			", renderStartccf4 varchar (32)" .
			", renderStartccv4 float unsigned" .
			", renderStartccf5 varchar (32)" .
			", renderStartccv5 float unsigned" .

			", primary key (label, slice, device)" .
			") ENGINE MyISAM;";
		doSimpleCommand($command);
	}

	// Create Urls Table
	if ( ! tableExists($gUrlsTable) ) {
		$command = "create table $gUrlsTable (" .
			"urlhash int(8) unsigned" .
			", urlOrig text character set binary" . // use "binary" so it's case *sensitive*
			", urlFixed text" .
			", rank int(10) unsigned" .
			", ranktmp int(10) unsigned" . // use this while we're updating the rank every night
			", other boolean not null" .   // is this URL from some other source and we should crawl it EVERY time
			", optout boolean not null" .  // did the website owner ask us NOT to crawl their site
			", primary key (urlOrig(255))" .
			", index(urlhash)" .
			") ENGINE MyISAM;";
		doSimpleCommand($command);
	}

	// Create Urls Change Table
	// If someone chooses to have their site removed, we queue the request here.
	if ( ! tableExists($gUrlsChangeTable) ) {
		$command = "create table $gUrlsChangeTable (" .
			"url text character set binary" . // use "binary" so it's case *sensitive*
			", action varchar(16)" .  // "add", "remove" for now
			", createDate int(10) unsigned not null" .
			", primary key (url(255), action)" .
			") ENGINE MyISAM;";
		doSimpleCommand($command);
	}

	// Create Settings Table
	if ( ! tableExists($gSettingsTable) ) {
		$command = "create table $gSettingsTable (" .
			"setting varchar (64)" .
			", val varchar (255)" .
			", primary key (setting)" .
			") ENGINE MyISAM;";
		doSimpleCommand($command);
	}
}


function createRequestsTable($tablename) {
	global $ghReqHeaders, $ghRespHeaders;

	$sColumns = "";
	$aColumns = array_values($ghReqHeaders);
	sort($aColumns);
	for ( $i = 0; $i < count($aColumns); $i++ ) {
		$column = $aColumns[$i];
		$sColumns .= ", $column varchar (255)";
	}
	$aColumns = array_values($ghRespHeaders);
	sort($aColumns);
	for ( $i = 0; $i < count($aColumns); $i++ ) {
		$column = $aColumns[$i];
		$sColumns .= ", $column varchar (255)";
	}

	$command = "create table $tablename (" .
		"requestid int unsigned not null auto_increment" .
		", pageid int unsigned not null" .

		", startedDateTime int(10) unsigned" .
		", time int(10) unsigned" .
		", method varchar (32)" .
		", url text" .
		", urlShort varchar (255)" .
		", redirectUrl text" .
		", firstReq tinyint(1) not null" .
		", firstHtml tinyint(1) not null" .

		// req
		", reqHttpVersion varchar (32)" .
		", reqHeadersSize int(10) unsigned" .
		", reqBodySize int(10) unsigned" .
		", reqCookieLen int(10) unsigned not null".
		", reqOtherHeaders text" .

		// response
		", status int(10) unsigned" .
		", respHttpVersion varchar (32)" .
		", respHeadersSize int(10) unsigned" .
		", respBodySize int(10) unsigned" .
		", respSize int(10) unsigned" .
		", respCookieLen int(10) unsigned not null".
		", expAge int unsigned not null" .             // number of seconds until response expires
		", mimeType varchar(255)" .  // slightly normalized version of resp_content_type - ~1000 distinct values
		", respOtherHeaders text" .

		// headers
		$sColumns .

		", primary key (requestid)" .
		", index(pageid)" .
		", unique key (startedDateTime, pageid, urlShort)" .
		") ENGINE MyISAM;";

	doSimpleCommand($command);
}
?>
