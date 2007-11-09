<?

	$forceserver = 1;
	chdir("/home/nexopia/");
	require_once("include/general.lib.php");

	$res = $usersdb->query("SELECT * FROM usersearch");
	
	$str = "";
	
	$line = $res->fetchrow();
	echo implode(",", array_keys($line)) . "\n";
	echo implode(",", $line) . "\n";
	
	while($line = $res->fetchrow()){
		$line['sex'] = ($line['sex'] == 'Male' ? 0 : 1);
		echo implode(",", $line) . "\n";
	}

