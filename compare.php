<?

	$scriptname = array_shift($argv);

	if(count($argv) < 2)
		die("Bad Arguments: $scriptname originalfile newfile\n");

	$file1 = array_shift($argv);
	$file2 = array_shift($argv);
	

	$f1 = fopen($file1, 'r');
	$f2 = fopen($file2, 'r');

//drop the header line
	echo "trim1: " . fgets($f1);

	echo "trim2: " . fgets($f2);
	echo "trim2: " . fgets($f2);
	echo "trim2: " . fgets($f2);

	$i = 1;

	while(1){
		$line1 = trim(fgets($f1));
		$line2 = trim(fgets($f2));

		if(!$line1 && !$line2)
			break;

		if(substr($line1, strpos($line1, ",")+1) != $line2)
			die("Differ on line $i\n$line1\n$line2\n");

		$i++;
	}

	die("No Difference\n");
