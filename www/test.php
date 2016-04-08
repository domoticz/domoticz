<html>
 <head>
  <title>PHP Test</title>
 </head>
 <body>
 <?php
	echo '<p>Hello World</p>';
	
	if (isset($_GET['rob']) )
	{
		$rob = $_GET['rob'];
		echo 'rob='.$rob;
	} else {
		echo "No rob!";
	}

	echo phpinfo();
?>
 </body>
</html>
