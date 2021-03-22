<?php
$to = "yzhong@hawaii.edu, cmora@hawaii.edu, kylemcdo@hawaii.edu";
$subject = "IPS-System Temperature Service"; //主题
$message = "Hi guys,the third version of IPS has been deployed to the server and the temperature feature is available now. It currently reports temperature data every ten minutes. For more information please view the log on the website.https://uh-ips.000webhostapp.com/. Please do not reply to this email, It was sent from an address that cannot accept incoming email."; //正文
mail($to,$subject,$message);
?>
