<?

class usersearchclient {
	public $host;
	public $port;
	public $returnheaders;
	
	function __construct($host, $port){
		$this->host = $host;
		$this->port = $port;
		$this->returnheaders = false;
		$this->timeout = 0.05; //50ms should be tons
	}

	function addUser($params){
		if(!isset($params['userid']) ||
		   !isset($params['age']) ||
		   !isset($params['sex']) ||
		   !isset($params['loc']) ||
		   !isset($params['active']) ||
		   !isset($params['pic']) ||
		   !isset($params['single']) ||
		   !isset($params['sexuality'])
		  ){
			trigger_error("Badly formed params", E_USER_WARNING);
			return false;
		}

		return ($this->cmd("adduser", $params) == "SUCCESS");
	}

	function updateUser($userid, $params){
		if(!count($params))
			return false;

		$params['userid'] = $userid;

		return ($this->cmd("updateuser", $params) == "SUCCESS");
	}
	
	function deleteUser($userid){
		return ($this->cmd("deleteuser", array("userid" => $userid)) == "SUCCESS");
	}

	function search($params){
		$str = $this->cmd("search", $params);

		$lines = explode("\n", trim($str));

		$results = array();

		$results['userids'] = explode(",", trim(array_pop($lines), ','));

		foreach($lines as $line){
			list($k, $v) = explode(": ", $line);
			$results[$k] = $v;
		}

		return $results;
	}

	function prepare($cmd, $params = array()){
		$uri = ($cmd[0] == '/' ? $cmd : "/$cmd");

		if($params){
			$sep = (strpos($uri, '?') === false ? '?' : '&');

			foreach($params as $k => $v){
				$uri .= "$sep" . urlencode($k) . "=" . urlencode($v);
				$sep = '&';
			}
		}

		return $uri;
	}

	function cmd($cmd, $params = array()){
		$uri = $this->prepare($cmd, $params);

		$errno = null;
		$errstr = null;

		$sock = fsockopen($this->host, $this->port, $errno, $errstr, $this->timeout);

		if(!$sock)
			return "";

		fwrite($sock, "GET $uri HTTP/1.0\r\n\r\n");

		$str = "";
		$len = 0;
		while($header = fgets($sock, 1024)){
			$str .= $header;
			if(substr($header, 0, 16) == "Content-Length: ")
				$len = substr($header, 16);
			if(trim($header) == '')
				break;
		}

		if(!$this->returnheaders)
			$str = "";

		//use $len instead of just reading till the sock closes?
		while(!feof($sock))
			$str .= fread($sock, 1024);
		fclose($sock);

		return $str;
	}
}

