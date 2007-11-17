#!/usr/bin/php
<?

$filename = array_shift($argv);

if(count($argv) < 2)
	die("Bad arguments: host port\n");

$host = array_shift($argv);
$port = array_shift($argv);


include("client.php");

$client = new usersearchclient($host, $port);

while($line = readline()){
	if(substr($line, 0, 2) == '> ') //allow a full line copy from a previous command
		$line = substr($line, 2);

	if($line[0] == '/') //strip the /, as it's added by the client
		$line = substr($line, 1);

	$cmd = $line;
	$params = "";

	if($pos = strpos($line, " ")){
		$cmd = substr($line, 0, $pos);
		$params = substr($line, $pos+1);
	}

	switch($cmd){
		case 'quit':
			break 2;

		case 'showheaders':
			$client->returnheaders = true;
			break;

		case 'hideheaders':
			$client->returnheaders = false;
			break;

		case 'clientbench':
			list($count, $cmd) = explode(" ", $params, 2);

			$start = microtime(true);
			for($i = 0; $i < $count; $i++)
				$client->cmd($cmd, array());
			$end = microtime(true);

			echo "Num runs:     " . number_format($count) . "\n";
			echo "Total time:   " . number_format(($end-$start), 2) . " s\n";
			echo "Avg run time: " . number_format(($end-$start)*1000/$count, 2) . " ms\n";
			echo "Requests/s:   " . number_format($count/(($end-$start)), 2) . "\n";
			break;

		case 'test':
			$count = $params;

			$start = microtime(true);

			for($i = 0; $i < $count; $i++){
				if($i && $count > 100 && $i % ($count/10) == 0)
					echo "$i\n";
				$uid = rand(1, 2000000);

				$check = $client->cmd("printuser?userid=$uid");

				if(trim($check) == 'Bad Userid'){ //user doesn't exist, add it
					$client->addUser( array(
						'userid' => $uid,
						'age' => rand(14,60),
						'sex' => rand(0,1),
						'loc' => rand(1,50),
						'active' => rand(0,2),
						'pic' => rand(0,2),
						'single' => rand(0,1),
						'sexuality' => rand(0,3),
						)
					);
				}elseif(rand(1,10) == 1){ //10% chance to delete
					$client->deleteUser($uid);
				}else{ //90% to update
					$params = array();
					do{
						if(rand(1,3) == 1) $params['age'] = rand(14,60);
						if(rand(1,3) == 1) $params['sex'] = rand(0,1);
						if(rand(1,3) == 1) $params['loc'] = rand(1,50);
						if(rand(1,3) == 1) $params['active'] = rand(0,2);
						if(rand(1,3) == 1) $params['pic'] = rand(0,2);
						if(rand(1,3) == 1) $params['single'] = rand(0,1);
						if(rand(1,3) == 1) $params['sexuality'] = rand(0,3);
					}while(!$params);

					$client->updateUser($uid, $params);
				}

			//search
				$params = array();

				if(rand(1,3) >= 2){$params['agemin'] = rand(14,30); $params['agemax'] = $params['agemin'] + rand(0,15); }
				if(rand(1,2) == 1) $params['sex'] = rand(0,2);
				if(rand(1,3) == 1) $params['loc'] = rand(1,50);
				if(rand(1,2) == 1) $params['active'] = rand(0,2);
				if(rand(1,2) == 1) $params['pic'] = rand(0,2);
				if(rand(1,3) == 1) $params['single'] = rand(0,1);
				if(rand(1,5) == 1) $params['sexuality'] = rand(0,3);

				$results = $client->search($params);
			}

			$end = microtime(true);

			echo "Num runs:     " . number_format($count) . "\n";
			echo "Total time:   " . number_format(($end-$start), 2) . " s\n";
			echo "Avg run time: " . number_format(($end-$start)*1000/$count, 2) . " ms\n";
			echo "Requests/s:   " . number_format($count/(($end-$start)), 2) . "\n";
			break;

			break;

		case 'ab':
			$parts = explode(" ", $params);
			$uri = array_pop($parts);
			if($uri[0] == '/')
				$uri = substr($uri, 1);
			$parts[] = "http://$host:$port/$uri";

			echo shell_exec("ab " . implode(" ", $parts));
			break;

		case 'printuser':
			echo $client->cmd($cmd, array('userid' => $params));
			break;

		default:
			echo $client->cmd($line, array());
	}
}
echo "\n";

function readline(){
//	static $hist = array();
	$line = "";

//	end($hist);

	while(!feof(STDIN)){
		echo "> ";
		$line = fgets(STDIN, 1024);

		$line = trim($line);

		if($line == "")
			continue;

//		$hist[] = $line;
		return $line;
	}
	return "";
}
