SELECT 
    time,
    latitude,
    longitude,
    altitude,
    dr,
    (e.tx_info->'frequency')::INT AS frequency,
    (e.tx_info->'modulation'->'lora'->>'spreadingFactor')::INT AS spreading_factor,
    (e.tx_info->'modulation'->'lora'->>'bandwidth')::INT AS bandwidth,
    (rx_info_element->>'snr')::NUMERIC(10, 6) AS snr,
    (rx_info_element->>'rssi')::INT AS rssi,
    (rx_info_element->>'channel')::INT AS channel,
    CASE
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0ba786' THEN 'CC2'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0fcd08' THEN 'LW4'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0916f5' THEN '(OLD LW2 V1 IN R2)'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0fcd06' THEN 'LW2'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0ba74f' THEN 'LW1'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0ba749' THEN 'LW5'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0ba78a' THEN 'LW3'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0fcd04' THEN 'CC3'
        ELSE rx_info_element->>'gatewayId'
    END AS gateway_id
FROM 
    car_merged_results AS e,
    jsonb_array_elements(e.rx_info) AS rx_info_element
WHERE 
    e.f_port = 110 AND
    (CASE 
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0ba786' THEN 'CC2'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0fcd08' THEN 'LW4'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0916f5' THEN '(OLD LW2 V1 IN R2)'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0fcd06' THEN 'LW2'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0ba74f' THEN 'LW1'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0ba749' THEN 'LW5'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0ba78a' THEN 'LW3'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0fcd04' THEN 'CC3'


    END) IN ($gateway)
    AND (tx_info->'modulation'->'lora'->>'spreadingFactor')::int IN ($spreading_factor);
