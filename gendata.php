<?

	$forceserver = 1;
//	@chdir("/home/nexopia/public_html/");
	require_once("include/general.lib.php");

	set_time_limit(0);

	$ip = getip();
	if(substr($ip, 0, 3) != "10.")
		die("Permission Denied");

	header("Content-type", "text/text");

	$dumpdbs = $usersdb->getSplitDBs();

	if(getREQval('small', 'bool'))
		$dumpdbs = array($dumpdbs[5]);
	
	switch($action){
		case "locations":
			echo "id,parent\n";

			$res = $configdb->query("SELECT id, parent FROM locations");
			
			while($line = $res->fetchrow())
				echo implode(",", $line) . "\n";

			break;

		case "users":
			echo "userid,age,sex,loc,active,pic,single,sexuality\n";

			$query = $usersdb->prepare("SELECT userid, age, IF(sex = 'Male', 0, 1) as sex, loc, ( (activetime > ?) + (activetime > ?) + (online = 'y' && activetime > ?) ) as active, ( (firstpic >= 1) + (signpic = 'y') ) as pic, (single = 'y') as single, sexuality FROM users", 
				time() - 86400*30, time() - 86400*7, time() - 600);

			$str = "";

			foreach($dumpdbs as $userdb){
				$res = $userdb->unbuffered_query($query);

				while($line = $res->fetchrow()){
					$str .= implode(",", $line) . "\n";
					if(strlen($str) > 10000){
						echo $str;
						$str = "";
					}
				}
				if($small)
					break;
			}
			echo $str;

			break;

		case "interests":
			echo "userid,interestid\n";

			$str = "";

			foreach($dumpdbs as $userdb){
				$res = $userdb->unbuffered_query("SELECT userid,interestid FROM userinterests");

				while($line = $res->fetchrow()){
					$str .= implode(",", $line) . "\n";
					if(strlen($str) > 10000){
						echo $str;
						$str = "";
					}
				}
				if($small)
					break;
			}
			echo $str;

			break;
	}

