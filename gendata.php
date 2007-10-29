<?

	$forceserver = 1;
	chdir("/home/nexopia/");
	require_once("include/general.lib.php");

	$fp = fopen("search.txt", 'w');
	
	$res = $usersdb->query("SELECT * FROM usersearch");
	
	$str = "";
	
	$line = $res->fetchrow();
	$str .= implode(",", array_keys($line)) . "\n";
	$str .= implode(",", $line) . "\n";
	
	$i = 0;
	
	while($line = $res->fetchrow()){
		$str .= implode(",", $line) . "\n";

		if(++$i >= 10000){
			fwrite($fp, $str);
			$str = "";
			$i = 0;
		}
	}
	fwrite($fp, $str);
	fclose($fp);
