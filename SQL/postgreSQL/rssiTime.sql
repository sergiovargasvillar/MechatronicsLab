SELECT 
    time, 
    (CASE 
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0ba786' THEN 'CC2'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0fcd08' THEN 'LW4'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0916f5' THEN '(OLD LW2 V1 IN R2)'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0fcd06' THEN 'LW2'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0ba74f' THEN 'LW1'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0ba749' THEN 'LW5'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0ba78a' THEN 'LW3'
        WHEN rx_info_element->>'gatewayId' = 'ac1f09fffe0fcd04' THEN 'CC3'
    END) as gateway_name,
    (rx_info_element->>'rssi')::float AS rssi
FROM drone_merged_results,
    jsonb_array_elements(rx_info) AS rx_info_element
WHERE 
    f_port = 1
    AND
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
    AND (tx_info->'modulation'->'lora'->>'spreadingFactor')::int IN ($spreading_factor)
    ORDER BY time ASC;
