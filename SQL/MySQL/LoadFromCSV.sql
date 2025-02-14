LOAD DATA LOCAL INFILE "./persons.csv" INTO TABLE meta.persons
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
IGNORE 1 LINES
(pk_index, status, @datevar, first_name, last_name, fk_owner)
set version = STR_TO_DATE(@datevar,'%m/%d/%Y');

LOAD DATA LOCAL INFILE "./taxis.csv" INTO TABLE meta.taxis
FIELDS TERMINATED BY ','
LINES TERMINATED BY '\n'
IGNORE 1 LINES
(pk_index, status, @datevar, license_plate, fk_owner, internal_number, taxi_status, type, working_status, @taxi_driver_app_var, fk_person)
set version = STR_TO_DATE(@datevar,'%m/%d/%Y'),
		taxi_driver_app = CAST(@taxi_driver_app_var as signed);
