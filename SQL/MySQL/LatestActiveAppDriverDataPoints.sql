SELECT 
  * 
FROM 
  (
    SELECT 
      *, 
      ROW_NUMBER() OVER (
        PARTITION BY fk_taxi_taxi_driver 
        ORDER BY 
          last_modified_date DESC
      ) AS RowNum 
    FROM 
      data_points
  ) AS T 
WHERE 
  RowNum = 1 
  AND fk_taxi_taxi_driver in (
    SELECT 
      pk_index 
    FROM 
      taxi_taxi_drivers 
    WHERE 
      fk_driver in (
        SELECT 
          pk_index 
        FROM 
          taxi_drivers 
        where 
          pk_index in (
            SELECT 
              fk_driver 
            from 
              taxi_taxi_drivers 
            where 
              fk_taxi in (
                SELECT 
                  pk_index 
                FROM 
                  taxis 
                where 
                  working_status = 'WORKING' 
                  AND status != 'REMOVED'
              )
          ) 
          AND taxi_driver_app = 1
      )
  );
