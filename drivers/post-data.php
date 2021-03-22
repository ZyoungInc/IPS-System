<?php
header('Access-Control-Allow-Origin: *');//允许跨域调用-for debug
$api_key_value = "tPmAT6Ab3j7F9";

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $api_key = test_input($_POST["api_key"]);
    if($api_key == $api_key_value) {
        $Machine = test_input($_POST["Machine"]);
        $Pot = test_input($_POST["Pot"]);
        $Type = test_input($_POST["Type"]);
        $value = test_input($_POST["value"]);
        
        // Create connection
        require_once $_SERVER['DOCUMENT_ROOT'].'/drivers/loginDB.php';
        if($Pot==NULL){
            $sql = "INSERT INTO IPSChamData (Machine, Type, value)
            VALUES ('" . $Machine . "', '" . $Type . "', '" . $value . "')";

        }else{
            $sql = "INSERT INTO IPSPotData (Machine, Pot, Type, value)
            VALUES ('" . $Machine . "', '" . $Pot . "', '" . $Type . "', '" . $value . "')";
        }
        
        if ($conn->query($sql) === TRUE) {
            echo "New record created successfully";
        } 
        else {
            echo "Error: " . $sql . "<br>" . $conn->error;
        }
    
        $conn->close();
    }
    else {
        echo "Wrong API Key provided.";
    }

}
else {
    echo "No data posted with HTTP POST.";
}

function test_input($data) {
    $data = trim($data);
    $data = stripslashes($data);
    $data = htmlspecialchars($data);
    return $data;
}