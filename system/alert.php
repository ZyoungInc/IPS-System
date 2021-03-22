<?php
$servername = "localhost";
$dbname = "id12056398_test";
$username = "id12056398_test";
$password = "testtest";
$list = "yzhong@hawaii.edu, cmora@hawaii.edu, kylemcdo@hawaii.edu";
global $now,$end;
$now = date('Y-m-d H:i:s',time());
$time =date("Y-m-d",strtotime("-7days",strtotime(date('Y-m-d',time()))));//acceleration
$alarttime=3600;

$sql = "SELECT id, sensor, location, value1, value2, value3, reading_time FROM Machine1 WHERE reading_time>";
$sql .="'";
$sql .=$time;
$sql .="'";
$sql .=" ORDER BY id DESC";
$conn = new mysqli($servername, $username, $password, $dbname);
//set up the database configuration.
if ($conn->connect_error) {
    $subject = "IPS-System Alart: Datsbase Error"; //主题
    $message = "Hi guys,This is a automatic email alart. The Datsbase error, Please check. "; //正文
    mail($list,$subject,$message);
    die("Connection failed: " . $conn->connect_error);
} else {
  $result = $conn->query($sql);
  $row = $result->fetch_assoc();
  $end = $row["reading_time"];
  $result->free();
  $conn->close();
  //get data from database
  $diff = strtotime($now) - strtotime($end);
  $honolulu = date("Y-m-d H:i:s",strtotime("-10hours",strtotime($end)));
  echo '<br><br>Last response:&nbsp;';
  print_r($diff);
  echo '&nbsp;sec<br><br>Server time:&nbsp;';
  print_r($now);
  echo '&nbsp;&nbsp;(UTC time zone)<br><br>Last response time:&nbsp;';
  print_r($end);
  echo '&nbsp;&nbsp;(UTC time zone)<br><br>Last response time:&nbsp;';
  print_r($honolulu);
  //debug iformation
  $myfile = fopen("notification.txt", "r") or die("Unable to open file!");
  $status = fgets($myfile);//read a line
  fclose($myfile);
  echo '&nbsp;&nbsp;(Honolulu time zone)<br><br>Current status:&nbsp;';
  print_r($status);
  echo '<br><br>Operation status:&nbsp;';
    if($diff > $alarttime){
      echo 'Last response more than 1 hour from now.';
      if($status=="online"){
        $subject = "IPS-System Alart: Machine 1 offline"; //主题
        $message = "Hi guys,This is a automatic email alart. The Machine 1 was offline, Please check. The last response from machine is: ".$honolulu; //正文
        mail($list,$subject,$message);
        $myfile = fopen("notification.txt", "w") or die("Unable to open file!");
        fwrite($myfile, "offline");
        fclose($myfile);
        echo '<br>It was offline, the alert email sent.';
      }
    } else {
      echo '&nbspLast response less than 1 hour from now.';
      if($status=="offline"){
        $subject = "IPS-System Notification: Machine 1 back to online"; //主题
        $message = "Hi guys,This is a automatic email notification. The Machine 1 was back to online. The last response from machine is: ".$honolulu; //正文
        mail($list,$subject,$message);
        $myfile = fopen("notification.txt", "w") or die("Unable to open file!");
        fwrite($myfile, "online");
        fclose($myfile);
        echo '<br>It was online, the notification email sent.';
      }
    }
echo '<br><br>All complete.';

}
 ?>
