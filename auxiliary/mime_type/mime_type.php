<?
/*
 * x-world/x-vrml                                  vrm vrml wr
 */

$suffixs = Array();
$mts = explode("\n", str_replace("\r", "", file_get_contents("/etc/mime.types")));
foreach ($mts as $mt)
{
	$mt = trim($mt);
	if ($mt == "") {
		continue;
	}
	if($mt[0] == "#") {
		continue;
	}
	$mt = str_replace("\t", " ", $mt);
	$ss = explode(" ", $mt);
	if (count($ss) < 2) {
		continue;
	}
	$ct = $ss[0];
	for($i=1;$i<count($ss);$i++) {
		$suf = trim($ss[$i]);
		if ($suf == "") {
			continue;
		}
		$suffixs[$suf] = $ct;
	}
}
$skeys = array_keys($suffixs);
sort($skeys);

$output = "";
$output = "static const int mt_count = ". count($skeys) . ";\n";
$output .= "static const char *mt_vector = ";

$offsets = Array();
$mtstr = "";
$offset = 0;
foreach($skeys as $suf) {
	$offsets[] = $offset;
	$offset += strlen($suf) + 1 + strlen($suffixs[$suf]) + 1;
	$mtstr .= " \"".$suf ."\\x00\" \"" . $suffixs[$suf]. "\\x00\"";
}

$output .= $mtstr . ";\n";
$output .= "static const unsigned short int mt_offsets[] = {".join(",", $offsets)."};\n";

echo $output;
