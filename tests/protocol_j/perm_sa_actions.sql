/*!999999\- enable the sandbox mode */ 
-- MariaDB dump 10.19  Distrib 10.6.18-MariaDB, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: drouter
-- ------------------------------------------------------
-- Server version	10.6.18-MariaDB-0ubuntu0.22.04.1

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `PERM_SA_ACTIONS`
--

DROP TABLE IF EXISTS `PERM_SA_ACTIONS`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `PERM_SA_ACTIONS` (
  `ID` int(11) NOT NULL AUTO_INCREMENT,
  `IS_ACTIVE` enum('Y','N') DEFAULT 'Y',
  `TIME` time NOT NULL,
  `SUN` enum('N','Y') NOT NULL DEFAULT 'N',
  `MON` enum('N','Y') NOT NULL DEFAULT 'N',
  `TUE` enum('N','Y') NOT NULL DEFAULT 'N',
  `WED` enum('N','Y') NOT NULL DEFAULT 'N',
  `THU` enum('N','Y') NOT NULL DEFAULT 'N',
  `FRI` enum('N','Y') NOT NULL DEFAULT 'N',
  `SAT` enum('N','Y') NOT NULL DEFAULT 'N',
  `ROUTER_NUMBER` int(11) NOT NULL,
  `DESTINATION_NUMBER` int(11) NOT NULL,
  `SOURCE_NUMBER` int(11) NOT NULL,
  `COMMENT` text DEFAULT NULL,
  PRIMARY KEY (`ID`),
  KEY `ROUTER_NUMBER_IDX` (`ROUTER_NUMBER`),
  KEY `IS_ACTIVE_IDX` (`IS_ACTIVE`)
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8mb3 COLLATE=utf8mb3_general_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `PERM_SA_ACTIONS`
--

LOCK TABLES `PERM_SA_ACTIONS` WRITE;
/*!40000 ALTER TABLE `PERM_SA_ACTIONS` DISABLE KEYS */;
INSERT INTO `PERM_SA_ACTIONS` VALUES (1,'Y','12:00:00','Y','Y','Y','Y','Y','Y','Y',0,0,0,'Forward Xpoint 1'),(2,'N','00:00:00','N','N','N','N','N','N','N',0,3,0,'Null Event'),(3,'Y','12:00:00','Y','Y','Y','Y','Y','Y','Y',0,1,1,'Forward Xpoint 2'),(4,'Y','12:00:00','Y','Y','Y','Y','Y','Y','Y',0,2,2,'Forward Xpoint 3'),(5,'Y','12:00:00','Y','Y','Y','Y','Y','Y','Y',0,3,3,'Forward Xpoint 4'),(6,'Y','12:01:00','Y','Y','Y','Y','Y','Y','Y',0,0,3,'Reverse Xpoint 1'),(7,'Y','12:01:00','Y','Y','Y','Y','Y','Y','Y',0,1,2,'Reverse Xpoint 2'),(8,'Y','12:01:00','Y','Y','Y','Y','Y','Y','Y',0,2,1,'Reverse Xpoint 3'),(9,'Y','12:01:00','Y','Y','Y','Y','Y','Y','Y',0,3,0,'Reverse Xpoint 4');
/*!40000 ALTER TABLE `PERM_SA_ACTIONS` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2024-10-04 15:10:57
