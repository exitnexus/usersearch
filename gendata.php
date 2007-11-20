<?

	$forceserver = 1;
	chdir("/home/nexopia/public_html/");
	require_once("include/general.lib.php");

	$res = $usersdb->unbuffered_query("SELECT * FROM usersearch");

	$line = $res->fetchrow();
	echo implode(",", array_keys($line)) . "\n";
	do{
		$line['sex'] = ($line['sex'] == 'Male' ? 0 : 1);
		echo implode(",", $line) . "\n";
	}while($line = $res->fetchrow());

	echo "\n";

	$res = $usersdb->unbuffered_query("SELECT * FROM userinterests");

	$line = $res->fetchrow();
	echo implode(",", array_keys($line)) . "\n";
	do{
		echo implode(",", $line) . "\n";
	}while($line = $res->fetchrow());

