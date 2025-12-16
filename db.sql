
 CREATE TABLE `LeituraSensor` (
    `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
    `co2` INT NOT NULL DEFAULT 0,
    `temperatura` decimal(5,2) NOT NULL DEFAULT 0.00,
    `umidade` decimal(5,2) NOT NULL DEFAULT 0.00,
    `data_hora` datetime NOT NULL DEFAULT current_timestamp(),
    PRIMARY KEY (`id`),
    KEY `idx_data_hora` (`data_hora`)
);

alter TABLE `LeituraSensor`
add COLUMN `co2` INT NOT NULL DEFAULT 0 AFTER `id`,
add COLUMN `pm1_0` DECIMAL(6,2) NOT NULL DEFAULT 0.00 AFTER `umidade`,
add COLUMN `pm2_5` DECIMAL(6,2) NOT NULL DEFAULT 0.00 AFTER `pm1_0`,
add COLUMN `pm4_0` DECIMAL(6,2) NOT NULL DEFAULT 0.00 AFTER `pm2_5`,
add COLUMN `pm10_0` DECIMAL(6,2) NOT NULL DEFAULT 0.00 AFTER `pm4_0`,
add COLUMN `vocIndex` INT NULL DEFAULT 0 AFTER `pm10_0`,
add COLUMN `noxIndex` INT NULL DEFAULT 0 AFTER `vocIndex`;

alter TABLE `LeituraSensor`
modify COLUMN `pm1_0` DECIMAL(6,2) NOT NULL DEFAULT 0.00;