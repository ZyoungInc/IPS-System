<?php
require $_SERVER['DOCUMENT_ROOT'].'/drivers/loginDB.php';

$jsonArray = array();

        if ($result = $conn->query($sql)) {
            while ($row = $result->fetch_assoc()) {
              $jsonArrayItem = array();
              $jsonArrayItem[] = date('Y-m-d G:i:s', (strtotime($row["reading_time"])-10800-(strtotime($row["reading_time"]) % 30)));//300是格式化间隔
              //$jsonArrayItem[] =$row["reading_time"];
              $jsonArrayItem[] = $row["value"];
              array_push($jsonArray, $jsonArrayItem);
            }
            $result->free();
        }

        $json = json_encode(array(
            //"resultCode"=>200,
            //"message"=>"查询成功！",
            //"action"=>$action,
            //"Machine"=>$Machine,
            //"Pot"=>$Pot,
            //"Time"=>$Time,
            //"sql"=>$sql,
            "data"=>$jsonArray
        ),JSON_UNESCAPED_UNICODE);
        
        //转换成字符串JSON
        echo($json);
?>