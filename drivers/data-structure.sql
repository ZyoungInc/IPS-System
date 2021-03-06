CREATE TABLE IPSPotData (
    id INT(6) UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    Machine VARCHAR(30) NOT NULL,
    Pot VARCHAR(30) NOT NULL,
    Type VARCHAR(10),
    value VARCHAR(10),
    reading_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
)

CREATE TABLE IPSChamData (
    id INT(6) UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    Machine VARCHAR(30) NOT NULL,
    Type VARCHAR(30) NOT NULL,
    value VARCHAR(10),
    reading_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
)