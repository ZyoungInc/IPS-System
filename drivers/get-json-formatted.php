<?php
header('Access-Control-Allow-Origin: *');//允许跨域调用
$action = $_GET['action'];
$Machine = $_GET['Machine'];
$Pot = $_GET['Pot'];
$Time = $_GET['Time'];


    switch($action) {
        case 'Temperature':
            SearchIPSCham();
            break;
        case 'Weight':
            SearchIPSPot();
            break;
        case 'CO2':
            SearchIPSCham();
            break;
        case 'Light':
            SearchIPSCham();
            break;
    }
    

    function SearchIPSCham(){
        global $action,$Machine,$Pot,$Time;
        //查询表
        $sql = "SELECT Machine, Type, value, reading_time FROM IPSChamData WHERE Type='" . $action . "' AND Machine='" . $Machine . "' AND reading_time>'" . $Time . "' ORDER BY id ASC";
        require $_SERVER['DOCUMENT_ROOT'].'/drivers/output-json.php';
    }

    function SearchIPSPot(){
        global $action,$Machine,$Pot,$Time;
        //查询表
        $sql = "SELECT Machine, Type, Pot, value, reading_time FROM IPSPotData WHERE Type='" . $action . "' AND Pot='" . $Pot . "' AND Machine='" . $Machine . "' AND reading_time>'" . $Time . "' ORDER BY id ASC  LIMIT 500";
        require $_SERVER['DOCUMENT_ROOT'].'/drivers/output-json.php';
    }
    


?>