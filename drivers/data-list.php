<!DOCTYPE html>
<html><body>
<?php
require_once $_SERVER['DOCUMENT_ROOT'].'/drivers/loginDB.php';

$sql = "SELECT id, Machine, Pot, Type, value, reading_time FROM IPSPotData ORDER BY id DESC LIMIT 50";

echo '<table cellspacing="5" cellpadding="5">
      <tr> 
        <td>ID</td> 
        <td>Machine</td> 
        <td>Pot</td> 
        <td>Type</td> 
        <td>value</td> 
        <td>Timestamp</td> 
      </tr>';
 
if ($result = $conn->query($sql)) {
    while ($row = $result->fetch_assoc()) {
        $row_id = $row["id"];
        $row_Machine = $row["Machine"];
        $row_Pot = $row["Pot"];
        $row_Type = $row["Type"];
        $row_value = $row["value"];
        $row_reading_time = $row["reading_time"];
        // Uncomment to set timezone to - 1 hour (you can change 1 to any number)
        $row_reading_time = date("Y-m-d H:i:s", strtotime("$row_reading_time - 10 hours"));
      
        // Uncomment to set timezone to + 4 hours (you can change 4 to any number)
        //$row_reading_time = date("Y-m-d H:i:s", strtotime("$row_reading_time + 4 hours"));
      
        echo '<tr> 
                <td>' . $row_id . '</td> 
                <td>' . $row_Machine . '</td> 
                <td>' . $row_Pot . '</td> 
                <td>' . $row_Type . '</td> 
                <td>' . $row_value . '</td>
                <td>' . $row_reading_time . '</td> 
              </tr>';
    }
    $result->free();
}

$sql = "SELECT id, Machine, Type, value, reading_time FROM IPSChamData ORDER BY id DESC LIMIT 50";

echo '<table cellspacing="5" cellpadding="5">
      <tr> 
        <td>ID</td> 
        <td>Machine</td>  
        <td>Type</td> 
        <td>value</td> 
        <td>Timestamp</td> 
      </tr>';
 
if ($result = $conn->query($sql)) {
    while ($row = $result->fetch_assoc()) {
        $row_id = $row["id"];
        $row_Machine = $row["Machine"];
        $row_Type = $row["Type"];
        $row_value = $row["value"];
        $row_reading_time = $row["reading_time"];
        // Uncomment to set timezone to - 1 hour (you can change 1 to any number)
        $row_reading_time = date("Y-m-d H:i:s", strtotime("$row_reading_time - 3 hours"));
      
        // Uncomment to set timezone to + 4 hours (you can change 4 to any number)
        //$row_reading_time = date("Y-m-d H:i:s", strtotime("$row_reading_time + 4 hours"));
      
        echo '<tr> 
                <td>' . $row_id . '</td> 
                <td>' . $row_Machine . '</td>  
                <td>' . $row_Type . '</td> 
                <td>' . $row_value . '</td>
                <td>' . $row_reading_time . '</td> 
              </tr>';
    }
    $result->free();
}


$conn->close();
?> 
</table>
</body>
</html>