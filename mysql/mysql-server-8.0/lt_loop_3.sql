USE sbtest;
DROP PROCEDURE IF EXISTS LT_3;

DELIMITER $$

CREATE PROCEDURE LT_3()
BEGIN
	DECLARE y INT;
	DECLARE time INT;
	DECLARE rand INT;
	SET time = UNIX_TIMESTAMP(NOW());
	label1: LOOP
		SELECT id from sbtest1;
		SET y = UNIX_TIMESTAMP(NOW());
		IF y-time < 60 THEN
			ITERATE label1;
		END IF;
		LEAVE label1;
	END LOOP label1;
END$$

DELIMITER ;
