<?php

require_once("../utils.php");

// Load the URLs in urls.txt file into status table.
function LoadUrlFromFile() {
  global $locations, $gArchive, $gLabel;
  $urls = file('./urls.txt', FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
  if (count($urls) > 0) {
    echo "Checking directory to store HAR files ...\r\n";
    $dir = "./archives/$gArchive/$gLabel";
    if( !is_dir("$dir") ) {
      // recursively create the dir
      mkdir("$dir", 0777, true);
    }
  }

  foreach( $urls as $url ) {
    $url = trim($url);
    if(strlen($url)) {
      foreach ($locations as $location) {
        UpdateStatusTable($url, $location);
      }
    }
  }
}

// Insert a row into the status table.
function UpdateStatusTable($url, $loc) {
  global $gStatusTable, $gArchive, $gLabel;
  $now = time();
  $cmd = "REPLACE INTO $gStatusTable SET url = '$url' , location = '$loc', archive = '$gArchive', label = '$gLabel', status = 0, timeofLastChange = '$now', retry = 0, wptid = 'none', wptRetCode = '0', medianRun = 0;";
  //dprint("$cmd");
  doSimpleCommand($cmd);
}

// Obtain the tests with specified status code
function ObtainTestsWithCode($status) {
  global $gStatusTable;
  $query = "SELECT * FROM $gStatusTable WHERE status = $status;";
  return doQuery($query);
}

// Obtain the tests which are not with specified status code
function ObtainTestsWithNotCode($status) {
  global $gStatusTable;
  $query = "SELECT * FROM $gStatusTable WHERE status != $status;";
  return doQuery($query);
}

// Evalute if a query returns emptry result
function IsEmptyQuery($resource) {
  // Deal with the result from doQuery()
  if (NULL != $resource) {
    $num_row = mysql_num_rows($resource);
    if (NULL != $num_row)
      return false;
  }
  return true;
}

// Increment retry col
function IncrementRetry($pageid, $retry) {
  global $gStatusTable;
  $num = $retry + 1;
  $cmd = "UPDATE $gStatusTable SET retry = $num WHERE pageid = $pageid;";
  doSimpleCommand($cmd);
}

// Submit the unfinished tests to WPT
function SubmitTest(&$record) {
  global $gStatusTable;

  global $video;
  global $private;
  global $runs;
  global $server;
  global $docComplete;
  global $fvonly;

  $request = $server . 'runtest.php?f=xml&priority=6&url=' . urlencode($record['url']) . "&location={$record['location']}&runs=$runs";
  if( $private )
    $request .= '&private=1';
  if( $video )
    $request .= '&video=1';
  if( $docComplete )
    $request .= '&web10=1';
  if($fvonly)
    $request .= '&fvonly=1';

  $doc = new DOMDocument();
  if( $doc ) {
    $response = file_get_contents($request);
    if( strlen($response) ) {
      $doc->loadXML($response);
      $nodes = $doc->getElementsByTagName('statusCode');
      $code = (int)trim($nodes->item(0)->nodeValue);
      if( $code == 200 ) {
        // Update status col in status table
        $nodes = $doc->getElementsByTagName('testId');
        $id = trim($nodes->item(0)->nodeValue);
        $cmd = "UPDATE $gStatusTable SET status = 1, wptid = '$id' WHERE pageid = $record[pageid];";
        doSimpleCommand($cmd);
      }
    }
    unset( $doc );
  }
}

// Set status code in status table
function SetStatus($pageid, $status) {
  global $gStatusTable;
  $cmd = "UPDATE $gStatusTable SET status = $status WHERE pageid = $pageid;";
  doSimpleCommand($cmd);
}

// Check if the test is done on WPT server.
function CheckWPTStatus() {
  // Query the status table to get wpt id list
  global $server, $gStatusTable;
  $result = ObtainTestsWithCode(1);
  if ($result) {
    while ($row = mysql_fetch_assoc($result)) {
      $request = $server . "testStatus.php?f=xml&test=" . $row['wptid'];
      $doc = new DOMDocument();
      if ($doc) {
        $response = file_get_contents($request);
        if (strlen($response)) {
          $doc->loadXML($response);
          $nodes = $doc->getElementsByTagName('statusText');
          $statusTxt = trim($nodes->item(0)->nodeValue);
          echo "{$row['pageid']} $statusTxt\r\n";
          if (!strcmp("Test Complete", $statusTxt)) {
            // Update status code to 2
            SetStatus($row['pageid'], 2);
          }
        }
      }
      unset($doc);
    }
  }
}

// Create a file name given an url
function BuildFileName($url) {
    $file = trim($url, "\r\n\t \\/");
    $file = str_ireplace('http://', '', $file);
    $file = str_ireplace(':', '_', $file);
    $file = str_ireplace('/', '_', $file);
    $file = str_ireplace('\\', '_', $file);
    $file = str_ireplace('%', '_', $file);

    return $file;
}

// Obain the .har file for the median run
function FillTables() {
  global $server;
  $tests = ObtainTestsWithCode(3);
  if (!IsEmptyQuery($tests)) {
    while ($row = mysql_fetch_assoc($tests)) {
      $file = BuildFileName($row['url']);
      $fullPath = "./archives/{$row['archive']}/{$row['label']}/$file.har";
      if (strlen($file) && !is_file($fullPath)) {
        $request = $server . "export.php?test={$row['wptid']}&run={$row['medianRun']}&cached=0";
        $response = file_get_contents($request);
        if (strlen($response)) {
          file_put_contents($fullPath, $response);
          SetStatus($row['pageid'], 4);
          ImportHarFile($fullPath, $row);
        } else {
          // Error handling of status code 3
        }
      }
    }
  }
}

// Import a specific HAR file.
function ImportHarFile($filename, $result) {
  global $gPagesTable, $gRequestsTable;
  $json_text = file_get_contents($filename);
  $HAR = json_decode($json_text);
  if (NULL == $HAR) {
    // Error handling for status code 4
    dprint("ERROR: importEntries failed. Purging pageid $pageid");
    return;
  }
  SetStatus($result['pageid'], 5);
  $log = $HAR->{ 'log' };

  $pages = $log->{ 'pages' };
  $pagecount = count($pages);
  if ( 0 == $pagecount ) {
    dprint("ERROR: No pages found in file $filename");
    return false;
  }
  if ( 1 < $pagecount ) {
    dprint("ERROR: Only one page is expected per HAR file. This HAR file has " . count($pages) . " pages. Only the first page will be processed.\n");
  }

  // STEP 1: Create a partial "page" record so we get a pageid.
  t_mark('ImportPage');
  $pageid = ImportPage($pages[0], $filename, $result);
  t_aggregate('ImportPage');
  if ( $pageid ) {
    $entries = $log->{ 'entries' };
    // STEP 2: Create all the resources & associate them with the pageid.
    $firstUrl = "";
    $firstHtmlUrl = "";
    t_mark('ImportEntries');
    $bEntries = ImportEntries($entries, $pageid, $firstUrl, $firstHtmlUrl);
    t_aggregate('ImportEntries');
    if ( false === $bEntries ) {
			dprint("ERROR: importEntries failed. Purging pageid $pageid");
			purgePage($pageid);
		}
		else {
			// STEP 3: Go back and fill out the rest of the "page" record based on all the resources.
			t_mark('AggregateStats');
			$url = AggregateStats($pageid, $firstUrl, $firstHtmlUrl, $result);
			t_aggregate('AggregateStats');
			if ( false === $url ) {
				dprint("ERROR: aggregateStats failed. Purging pageid $pageid");
				purgePage($pageid);
			}
			else {
				return true;
			}
		}
	}

  return false;
}

// Import a website.
// MAJOR ASSUMPTION: THERE'S ONLY ONE PAGE PER HAR FILE!
// (otherwise, harviewer and har_to_pagespeed won't work)
function ImportPage($page, $filename, $result) {
  global $gPagesTable, $gRequestsTable;
  $archive = $result['archive'];
  $label = $result['label'];

	$now = time();
	$aTuples = array();
	$pageref = $page->{ 'id' };

	// Add all the insert tuples to an array.
	array_push($aTuples, "createDate = $now");
	array_push($aTuples, "harfile = '$filename'");
	$startedDateTime = strtotime($page->{ 'startedDateTime' });
	array_push($aTuples, "startedDateTime = $startedDateTime");
	array_push($aTuples, "archive = '" . mysqlEscape($archive) . "'");
	if ( $label ) {
		array_push($aTuples, "label = '" . mysqlEscape($label) . "'");
	}
	$title = mysqlEscape($page->{ 'title' });
	array_push($aTuples, "title = '$title'");

	$pageTimings = $page->{ 'pageTimings' };
	//$renderStart = $pageTimings->{ '_renderStart' };
	//if ( $renderStart && 0 < $renderStart ) {
	//	array_push($aTuples, "renderStart = $renderStart");
	//}
	$onContentLoaded = $pageTimings->{ 'onContentLoad' };
	if ( $onContentLoaded && 0 < $onContentLoaded ) {
		array_push($aTuples, "onContentLoaded = $onContentLoaded");
	}
	$onLoad = $pageTimings->{ 'onLoad' };
	if ( $onLoad && 0 < $onLoad ) {
		array_push($aTuples, "onLoad = $onLoad");
	}

	// Page Speed score
	t_mark('Page Speed');
	$output = array();
	$return_var = 128;
	exec("../har_to_pagespeed '$filename' 2>/dev/null", $output, $return_var);
	if ( 0 === $return_var ) {
		$totalScore = 0;
		$iScores = 0;
		$matches = array();
		for ( $i = 0; $i < count($output); $i++ ) {
			$line = $output[$i];
			if ( preg_match("/_(.*)_ \(score=([0-9]+)/", $line, $matches) &&
				 false === strpos($line, "Optimize images") ) {
				$totalScore += $matches[2];
				$iScores++;
			}
		}
		$overallScore = round($totalScore/$iScores);
		array_push($aTuples, "PageSpeed = $overallScore");
	}
	t_aggregate('Page Speed');

	$cmd = "REPLACE INTO $gPagesTable set " . implode(", ", $aTuples) . ";";
	//dprint("$cmd");
	doSimpleCommand($cmd);

	// get the pageid
	$cmd = "select pageid from $gPagesTable where startedDateTime = $startedDateTime and harfile = '$filename';";
	//dprint("$cmd");
	$pageid = doSimpleQuery($cmd);

	return $pageid;
}


// Import the requests within a website.
function ImportEntries($entries, $pageid, &$firstUrl, &$firstHtmlUrl) {
	global $gPagesTable, $gRequestsTable;
	global $ghReqHeaders, $ghRespHeaders;
	global $ghReqOtherHeaders, $ghRespOtherHeaders;

	for ( $i = 0; $i < count($entries); $i++ ) {
		$entry = $entries[$i];
		$aTuples = array();
		array_push($aTuples, "pageid = $pageid");
		array_push($aTuples, "startedDateTime = " . strtotime($entry->{ 'startedDateTime' }));
		array_push($aTuples, "time = " . $entry->{ 'time' });

		// REQUEST
		$request = $entry->{ 'request' };
		array_push($aTuples, "method = '" . $request->{ 'method' } . "'");
		array_push($aTuples, "reqHttpVersion = '" . $request->{ 'httpVersion' } . "'");
		$url = $request->{ 'url' };
		array_push($aTuples, "url = '" . mysqlEscape($url) . "'");
		$urlShort = substr($url, 0, 255);
		array_push($aTuples, "urlShort = '" . mysqlEscape($urlShort) . "'");
		$reqHeaderSize = $request->{ 'headersSize' };
		if ( $reqHeaderSize && 0 < $reqHeaderSize ) {
			array_push($aTuples, "reqHeaderSize = $reqHeaderSize");
		}
		$reqBodySize = $request->{ 'bodySize' };
		if ( $reqBodySize && 0 < $reqBodySize ) {
			array_push($aTuples, "reqBodySize = $reqBodySize");
		}

		$headers = $request->{ 'headers' };
		$other = "";
		$hHeaders = array();  // Headers can appear multiple times, so we have to concat them all then add them to avoid setting a column twice.
		$cookielen = 0;
		for ( $h = 0; $h < count($headers); $h++ ) {
			$header = $headers[$h];
			$name = $header->{ 'name' };
			$lcname = strtolower($name);
			$value = substr($header->{ 'value' }, 0, 255);
			if ( array_key_exists($lcname, $ghReqHeaders) ) {
				$column = $ghReqHeaders[$lcname];
				$hHeaders[$column] = ( array_key_exists($column, $hHeaders) ? $hHeaders[$column] . " $value" : $value );
			}
			else if ( "cookie" == $lcname ) {
				$cookielen += strlen($value);
			}
			else {
				$other .= ( $other ? ", " : "" ) . "$name = $value";
				$ghReqOtherHeaders[$name] = ( array_key_exists($name, $ghReqOtherHeaders) ? $ghReqOtherHeaders[$name]+1 : 1 );
			}
		}
		if ( $other ) {
			array_push($aTuples, "reqOtherHeaders = '" . mysqlEscape($other) . "'");
		}
		if ( $cookielen ) {
			array_push($aTuples, "reqCookieLen = $cookielen");
		}

		// RESPONSE
		$response = $entry->{ 'response' };
		$status = $response->{ 'status' };
		array_push($aTuples, "status = $status");
		array_push($aTuples, "respHttpVersion = '" . $response->{ 'httpVersion' } . "'");
    if (property_exists($response, 'url')) {
		  array_push($aTuples, "redirectUrl = '" . mysqlEscape($response->{ 'url' }) . "'");
		  array_push($aTuples, "redirectUrlShort = '" . mysqlEscape(substr($response->{ 'url' }, 0, 255)) . "'");
    }
		$respHeaderSize = $response->{ 'headersSize' };
		if ( $respHeaderSize && 0 < $respHeaderSize ) {
			array_push($aTuples, "respHeaderSize = $respHeaderSize");
		}
		$respBodySize = $response->{ 'bodySize' };
		if ( $respBodySize && 0 < $respBodySize ) {
			array_push($aTuples, "respBodySize = $respBodySize");
		}
		$content = $response->{ 'content' };
		array_push($aTuples, "respSize = " . $content->{ 'size' });
		array_push($aTuples, "mimeType = '" . mysqlEscape($content->{ 'mimeType' }) . "'");
	
		$headers = $response->{ 'headers' };
		$other = "";
		$cookielen = 0;
		for ( $h = 0; $h < count($headers); $h++ ) {
			$header = $headers[$h];
			$name = $header->{ 'name' };
			$lcname = strtolower($name);
			$value = substr($header->{ 'value' }, 0, 255);
			if ( array_key_exists($lcname, $ghRespHeaders) ) {
				$column = $ghRespHeaders[$lcname];
				$hHeaders[$column] = ( array_key_exists($column, $hHeaders) ? $hHeaders[$column] . " $value" : $value );
			}
			else if ( "set-cookie" == $lcname ) {
				$cookielen += strlen($value);
			}
			else {
				$other .= ( $other ? ", " : "" ) . "$name = $value";
				$ghRespOtherHeaders[$name] = ( array_key_exists($name, $ghRespOtherHeaders) ? $ghRespOtherHeaders[$name]+1 : 1 );
			}
		}
		if ( $other ) {
			array_push($aTuples, "respOtherHeaders = '" . mysqlEscape($other) . "'");
		}
		if ( $cookielen ) {
			array_push($aTuples, "respCookieLen = $cookielen");
		}

		// NOW add all the headers from both the request and response.
		$aHeaders = array_keys($hHeaders);
		for ( $h = 0; $h < count($aHeaders); $h++ ) {
			$header = $aHeaders[$h];
			array_push($aTuples, "$header = '" . mysqlEscape($hHeaders[$header]) . "'");
		}

		$bFirstReq = 0;
		$bFirstHtml = 0;
		if ( ! $firstUrl ) {
			if ( 400 <= $status && $status <= 499 ) {
				dprint("ERROR: The first request ($url) failed with status $status.");
				return false;
			}
			// This is the first URL found associated with the page - assume it's the base URL.
			$bFirstReq = 1;
			$firstUrl = $url;
		}
		if ( ! $firstHtmlUrl && 200 == $status ) {
			// This is the first URL found associated with the page that's HTML.
			$bFirstHtml = 1;
			$firstHtmlUrl = $url;
		}
		array_push($aTuples, "firstReq = $bFirstReq");
		array_push($aTuples, "firstHtml = $bFirstHtml");

		$cmd = "REPLACE INTO $gRequestsTable SET " . implode(", ", $aTuples) . ";";
		//dprint("$cmd");
		doSimpleCommand($cmd);
	}
}

// Collect all the aggregate stats for a single website.
function AggregateStats($pageid, $firstUrl, $firstHtmlUrl, $resultTxt) {
	global $gPagesTable, $gRequestsTable;

	$bytesTotal = 0;
	$reqTotal = 0;
	$hSize = array();
	$hCount = array();
	foreach(array("flash", "css", "image", "script", "html", "other") as $type) {
		// initialize the hashes
		$hSize[$type] = 0;
		$hCount[$type] = 0;
	}
	$hDomains = array();

	t_mark('aggregateStats query');
    $result = doQuery("select mimeType, urlShort, respSize from $gRequestsTable where pageid = $pageid;");
	t_aggregate('aggregateStats query');
	while ($row = mysql_fetch_assoc($result)) {
		$mimeType = prettyMimetype($row['mimeType']);
		$respSize = intval($row['respSize']);
		$reqTotal++;
		$bytesTotal += $respSize;
		$hCount[$mimeType]++;
		$hSize[$mimeType] += $respSize;

		// count unique domains (really hostnames)
		$url = $row['urlShort'];
		$aMatches = array();
		if ( $url && preg_match('/http[s]*:\/\/([^\/]*)/', $url, $aMatches) ) {
			$hostname = $aMatches[1];
			$hDomains[$hostname] = 1; // don't need to count, just find unique domains
		}
		else { 
			dprint("ERROR: No hostname found in URL: $url");
		}
	}
	mysql_free_result($result);
	$numDomains = count(array_keys($hDomains));

	// CVSNO - move this error checking to the point before this function is called
	if ( ! $firstUrl ) {
		dprint("ERROR: no first URL found for pageref $pageref.");
		return false;
	}
	$url = $firstUrl;
	$urlShort = substr($url, 0, 255);

	if ( ! $firstHtmlUrl ) {
		dprint("ERROR: no first HTML URL found for pageref $pageref.");
		return false;
	}
	$urlHtml = $firstHtmlUrl;
	$urlHtmlShort = substr($urlHtml, 0, 255);
		
	$cmd = "update $gPagesTable set url = '$url', urlShort = '$urlShort', urlHtml = '$urlHtml', urlHtmlShort = '$urlHtmlShort', reqTotal = $reqTotal, bytesTotal = $bytesTotal" .
		", reqHtml = " . $hCount['html'] . ", bytesHtml = " . $hSize['html'] . 
		", reqJS = " . $hCount['script'] . ", bytesJS = " . $hSize['script'] . 
		", reqCSS = " . $hCount['css'] . ", bytesCSS = " . $hSize['css'] . 
		", reqImg = " . $hCount['image'] . ", bytesImg = " . $hSize['image'] . 
		", reqFlash = " . $hCount['flash'] . ", bytesFlash = " . $hSize['flash'] . 
		", reqOther = " . $hCount['other'] . ", bytesOther = " . $hSize['other'] . 
		", numDomains = $numDomains" .
		", wptid = '" . $resultTxt['wptid'] . "', wptrun = " . $resultTxt['medianRun'] . ", renderStart = " . $resultTxt['startRender'] .
		" where pageid = $pageid;";
	//dprint($cmd);
	doSimpleCommand($cmd);

	return $url;
}

// Obtain XML file of the test result
function ObtainXMLResult() {
  global $server, $gStatusTable;
  $tests = ObtainTestsWithCode(2);
  if (!IsEmptyQuery($tests)) {
    while ($row = mysql_fetch_assoc($tests)) {
      // Fetch the result xml file
      $doc = new DOMDocument();
      if ($doc) {
        $request = $server . "xmlResult/" . $row['wptid'] . "/";
        $response = file_get_contents($request);
        if (strlen($response)) {
          $doc->loadXML($response);
          $nodes = $doc->getElementsByTagName('statusCode');
          $status = trim($nodes->item(0)->nodeValue);
          $cmd = "UPDATE $gStatusTable SET wptRetCode = $status where pageid = $row[pageid];";
          doSimpleCommand($cmd);
          if (200 == $status) {
            // Import a website
            $result = array();
            ParseXMLResult($doc, $result);
            // Record medianRun in status table
            // TODO (zhaoq): Set wptRetCode
            $cmd = "UPDATE $gStatusTable SET medianRun = $result[medianRun], startRender = $result[startRender] WHERE pageid = $row[pageid];";
            doSimpleCommand($cmd);
            SetStatus($row['pageid'], 3);
          }
        } else {
          // Error handling for status code 2
        }
      }
    }
  }
}

function t_mark($name) {
	global $gMarks;
	$gMarks[$name] = time();
}

function t_measure($name) {
	global $gMarks;
	return ( array_key_exists($name, $gMarks) ? time() - $gMarks[$name] : 0 );
}

function t_aggregate($name) {
	global $gAggTimes, $gAggCounts;

	$delta = t_measure($name);
	if ( ! array_key_exists($name, $gAggTimes) ) {
		$gAggTimes[$name] = 0;
		$gAggCounts[$name] = 0;
	}

	$gAggTimes[$name] += $delta;
	$gAggCounts[$name]++;
}

function t_echo($name) {
	echo "$name: " . t_measure($name) . "\n";
}

function t_echoagg() {
	global $gAggTimes, $gAggCounts;

	foreach(array_keys($gAggTimes) as $key) {
		echo "$key: total=" . $gAggTimes[$key] . ", avg=" . round($gAggTimes[$key]/$gAggCounts[$key]) . "\n";
	}
}

function ParseXMLResult($doc, &$result)
{
    $runs = $doc->getElementsByTagName('run');

    $times = array();
    $failed = array();

    // pull the stuff we care about out from each run
    foreach( $runs as $run )
    {
        $index = (int)$run->getElementsByTagName('id')->item(0)->nodeValue;
        $fv = $run->getElementsByTagName('firstView');
        if( $fv )
        {
            $testResult = (int)$run->getElementsByTagName('result')->item(0)->nodeValue;
            $loadTime = (int)$run->getElementsByTagName('docTime')->item(0)->nodeValue;
            $render = (int)$run->getElementsByTagName('render')->item(0)->nodeValue;

            if( $loadTime && ($testResult == 0 || $testResult == 99999) )
                $times[$loadTime] = array('index' => $index, 'result' => $testResult, 'loadTime' => $loadTime, 'startRender' => $render);
            else
                $failed[] = array('index' => $index, 'result' => $testResult, 'loadTime' => $loadTime, 'startRender' => $render);
            unset($fv);
        }
    }

    // pull out the results we need
    if( count($times) )
    {
        // find the test with the median load time
        ksort($times);
        $i = (int)floor(count($times) / 2);
        $current = 0;
        foreach( $times as $index => &$data )
        {
            if( $current == $i )
            {
                $result['result'] = $data['result'];
                $result['medianRun'] = $data['index'];
                $result['loadTime'] = $data['loadTime'];
                $result['startRender'] = $data['startRender'];

                break;
            }
            $current++;
        }
    }
    elseif( count($failed) )
    {
        // test failed, grab the first result
        $result['result'] = $failed[0]['result'];
        $result['medianRun'] = $failed[0]['index'];
        $result['loadTime'] = $failed[0]['loadTime'];
        $result['startRender'] = $failed[0]['startRender'];
    }
}

// Parse out the pithy mime type from the long HTTP response header.
function prettyMimetype($mimeType) {
	$mimeType = strtolower($mimeType);

	// do most unique first
	foreach(array("flash", "css", "image", "script", "html") as $type) {
		if ( false !== strpos($mimeType, $type) ) {
			return $type;
		}
	}

	return "other";
}

?>