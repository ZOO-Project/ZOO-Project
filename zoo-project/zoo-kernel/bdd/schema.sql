create table status (
    id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    uuid TEXT,
    status TEXT,
    result TEXT,
    created_time DATETIME,
    start_date DATETIME,
    end_date DATETIME,
    progress int,
    info TEXT
    );


