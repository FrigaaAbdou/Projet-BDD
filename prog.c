/**************************************************************
 *  Air Quality DB Management - Terminal Version
 *  Demonstrates basic CRUD without any UI/GTK code
 **************************************************************/

 #include <mysql/mysql.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 
 /* ------------------ MySQL Connection Info ------------------ */
 #define DB_HOST     "localhost"
 #define DB_USER     "root"
 #define DB_PASS     "abdoudz1"
 #define DB_NAME     "airqualitydb"
 #define DB_PORT     3306
 
 /* ------------------ Global Variables ------------------ */
 MYSQL *conn = NULL;
 
 /* ------------------ Data Structures ------------------ */
 typedef struct {
     int    sensorID;
     char   location[101];   // "Bordeaux", "Paris (75)", etc.
     char   gasType[51];     // "CO2", "CH4", etc.
     int    isActive;        // 1 = active, 0 = disabled
     float  lastKnownValue;
 } Sensor;
 
 /* ------------------ Prototypes ------------------ */
 MYSQL* connectDB();
 void   closeDB(MYSQL *conn);
 void   createTables(MYSQL *conn);
 
 /* CRUD for 'sensors' */
 int    addSensorDB(const char *location, const char *gasType, float lastVal);
 int    updateSensorDB(int sensorId, const char *location, const char *gasType, float lastVal);
 int    deleteSensorDB(int sensorId);
 int    toggleSensorActiveDB(int sensorId, int newStatus);
 
 /* Insert a reading (optional) */
 int    insertReadingDB(int sensorId, float value);
 
 /* Reading (fetch) */
 Sensor* getAllSensors(int *count);  // renvoie un tableau dynamique + nb d'éléments
 void    freeSensorArray(Sensor *arr);
 
 /* Menu logic */
 void showMenu();
 void handleUserChoice(int choice);
 
 int main() {
     // Connexion à la base
     conn = connectDB();
     // Création des tables (si pas déjà existantes)
     createTables(conn);
 
     int choice = -1;
     while (choice != 0) {
         showMenu();
         printf("Votre choix : ");
         scanf("%d", &choice);
         getchar();  // Pour consommer le '\n' restant dans le buffer
         handleUserChoice(choice);
     }
 
     // Fermeture connexion
     closeDB(conn);
     printf("Bye!\n");
     return 0;
 }
 
 /* ------------------ Database Connection ------------------ */
 MYSQL* connectDB() {
     MYSQL *c = mysql_init(NULL);
     if (!c) {
         fprintf(stderr, "mysql_init() failed.\n");
         exit(1);
     }
     if (!mysql_real_connect(c, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, NULL, 0)) {
         fprintf(stderr, "Connection error: %s\n", mysql_error(c));
         mysql_close(c);
         exit(1);
     }
     return c;
 }
 
 void closeDB(MYSQL *c) {
     if (c) {
         mysql_close(c);
     }
 }
 
 void createTables(MYSQL *c) {
     // Table "sensors"
     const char *createSensorsQuery =
         "CREATE TABLE IF NOT EXISTS sensors ("
         "  sensorID INT AUTO_INCREMENT PRIMARY KEY,"
         "  location VARCHAR(100) NOT NULL,"
         "  gasType VARCHAR(50) NOT NULL,"
         "  isActive TINYINT NOT NULL DEFAULT 1,"
         "  lastKnownValue FLOAT DEFAULT 0"
         ") ENGINE=InnoDB;";
 
     if (mysql_query(c, createSensorsQuery)) {
         fprintf(stderr, "Error creating sensors table: %s\n", mysql_error(c));
     }
 
     // Table "readings"
     const char *createReadingsQuery =
         "CREATE TABLE IF NOT EXISTS readings ("
         "  readingID INT AUTO_INCREMENT PRIMARY KEY,"
         "  sensorID INT NOT NULL,"
         "  measureDate DATE NOT NULL,"
         "  measureValue FLOAT NOT NULL,"
         "  FOREIGN KEY (sensorID) REFERENCES sensors(sensorID) ON DELETE CASCADE"
         ") ENGINE=InnoDB;";
 
     if (mysql_query(c, createReadingsQuery)) {
         fprintf(stderr, "Error creating readings table: %s\n", mysql_error(c));
     }
 }
 
 /* ------------------ CRUD Operations (Sensors) ------------------ */
 int addSensorDB(const char *location, const char *gasType, float lastVal) {
     char query[512];
     snprintf(query, sizeof(query),
              "INSERT INTO sensors (location, gasType, lastKnownValue) "
              "VALUES ('%s','%s',%.2f);",
              location, gasType, lastVal);
 
     if (mysql_query(conn, query)) {
         fprintf(stderr, "Error adding sensor: %s\n", mysql_error(conn));
         return -1;
     }
     return (int)mysql_insert_id(conn);
 }
 
 int updateSensorDB(int sensorId, const char *location, const char *gasType, float lastVal) {
     char query[512];
     snprintf(query, sizeof(query),
              "UPDATE sensors SET location='%s', gasType='%s', lastKnownValue=%.2f "
              "WHERE sensorID=%d;",
              location, gasType, lastVal, sensorId);
 
     if (mysql_query(conn, query)) {
         fprintf(stderr, "Error updating sensor: %s\n", mysql_error(conn));
         return 0;
     }
     return (mysql_affected_rows(conn) > 0);
 }
 
 int deleteSensorDB(int sensorId) {
     char query[256];
     snprintf(query, sizeof(query), "DELETE FROM sensors WHERE sensorID=%d;", sensorId);
     if (mysql_query(conn, query)) {
         fprintf(stderr, "Error deleting sensor: %s\n", mysql_error(conn));
         return 0;
     }
     return (mysql_affected_rows(conn) > 0);
 }
 
 int toggleSensorActiveDB(int sensorId, int newStatus) {
     char query[256];
     snprintf(query, sizeof(query),
              "UPDATE sensors SET isActive=%d WHERE sensorID=%d;",
              newStatus, sensorId);
 
     if (mysql_query(conn, query)) {
         fprintf(stderr, "Error toggling sensor status: %s\n", mysql_error(conn));
         return 0;
     }
     return 1;
 }
 
 /* ------------------ Insert Reading (Optionnel) ------------------ */
 int insertReadingDB(int sensorId, float value) {
     time_t now = time(NULL);
     struct tm *tm_info = localtime(&now);
     char dateStr[11]; // "YYYY-MM-DD"
     strftime(dateStr, 11, "%Y-%m-%d", tm_info);
 
     char query[512];
     snprintf(query, sizeof(query),
              "INSERT INTO readings (sensorID, measureDate, measureValue) "
              "VALUES (%d, '%s', %.2f);",
              sensorId, dateStr, value);
 
     if (mysql_query(conn, query)) {
         fprintf(stderr, "Error inserting reading: %s\n", mysql_error(conn));
         return 0;
     }
     return (int)mysql_insert_id(conn);
 }
 
 /* ------------------ Fetch All Sensors ------------------ */
 Sensor* getAllSensors(int *count) {
     *count = 0;
     const char *query = "SELECT sensorID, location, gasType, isActive, lastKnownValue FROM sensors;";
     if (mysql_query(conn, query)) {
         fprintf(stderr, "Error fetching sensors: %s\n", mysql_error(conn));
         return NULL;
     }
     MYSQL_RES *res = mysql_store_result(conn);
     if (!res) return NULL;
 
     int numRows = mysql_num_rows(res);
     if (numRows == 0) {
         mysql_free_result(res);
         return NULL;
     }
     Sensor *arr = malloc(numRows * sizeof(Sensor));
     if (!arr) {
         fprintf(stderr, "Memory allocation error.\n");
         mysql_free_result(res);
         return NULL;
     }
 
     MYSQL_ROW row;
     int idx = 0;
     while ((row = mysql_fetch_row(res))) {
         arr[idx].sensorID       = atoi(row[0]);
         strncpy(arr[idx].location, row[1], 100);
         arr[idx].location[100]  = '\0';
         strncpy(arr[idx].gasType, row[2], 50);
         arr[idx].gasType[50]    = '\0';
         arr[idx].isActive       = atoi(row[3]);
         arr[idx].lastKnownValue = atof(row[4]);
         idx++;
     }
     mysql_free_result(res);
 
     *count = numRows;
     return arr;
 }
 
 void freeSensorArray(Sensor *arr) {
     if (arr) {
         free(arr);
     }
 }
 
 /* ------------------ Menu & Interaction ------------------ */
 void showMenu() {
     printf("\n================= Air Quality DB Menu =================\n");
     printf("1. Lister tous les capteurs\n");
     printf("2. Ajouter un capteur\n");
     printf("3. Mettre à jour un capteur\n");
     printf("4. Supprimer un capteur\n");
     printf("5. Activer/Désactiver un capteur\n");
     printf("6. Insérer un relevé (sur un capteur)\n");
     printf("0. Quitter\n");
     printf("=======================================================\n");
 }
 
 void handleUserChoice(int choice) {
     switch(choice) {
         case 1: {
             // Lister capteurs
             int count = 0;
             Sensor *arr = getAllSensors(&count);
             if (count == 0 || !arr) {
                 printf("Aucun capteur trouvé.\n");
             } else {
                 printf("Liste des capteurs (%d) :\n", count);
                 for (int i = 0; i < count; i++) {
                     printf("  ID=%d | %-15s | Gaz=%-6s | Actif=%d | DerniereValeur=%.2f ppm\n",
                            arr[i].sensorID, arr[i].location, arr[i].gasType,
                            arr[i].isActive, arr[i].lastKnownValue);
                 }
                 freeSensorArray(arr);
             }
         } break;
 
         case 2: {
             // Ajouter capteur
             char location[101], gasType[51];
             float value;
             printf("Entrez la localisation: ");
             scanf("%100s", location);
             printf("Entrez le type de gaz: ");
             scanf("%50s", gasType);
             printf("Entrez la derniere valeur connue (ppm): ");
             scanf("%f", &value);
 
             int newId = addSensorDB(location, gasType, value);
             if (newId > 0) {
                 printf("Capteur ajoute avec l'ID %d.\n", newId);
             } else {
                 printf("Echec de l'ajout.\n");
             }
         } break;
 
         case 3: {
             // Mettre a jour
             int id;
             char newLocation[101], newGasType[51];
             float newValue;
             printf("Entrez l'ID du capteur a mettre a jour: ");
             scanf("%d", &id);
             printf("Nouvelle localisation: ");
             scanf("%100s", newLocation);
             printf("Nouveau type de gaz: ");
             scanf("%50s", newGasType);
             printf("Nouvelle valeur (ppm): ");
             scanf("%f", &newValue);
 
             int success = updateSensorDB(id, newLocation, newGasType, newValue);
             if (success) {
                 printf("Capteur mis a jour avec succes.\n");
             } else {
                 printf("Echec de la mise a jour.\n");
             }
         } break;
 
         case 4: {
             // Supprimer capteur
             int id;
             printf("Entrez l'ID du capteur a supprimer: ");
             scanf("%d", &id);
             int success = deleteSensorDB(id);
             if (success) {
                 printf("Capteur supprime avec succes.\n");
             } else {
                 printf("Echec de la suppression.\n");
             }
         } break;
 
         case 5: {
             // Activer / desactiver
             int id;
             printf("Entrez l'ID du capteur: ");
             scanf("%d", &id);
             printf("Entrez 1 pour ACTIVER, 0 pour DESACTIVER: ");
             int newStatus;
             scanf("%d", &newStatus);
             int success = toggleSensorActiveDB(id, newStatus);
             if (success) {
                 printf("Capteur mis a jour (nouveau statut = %d).\n", newStatus);
             } else {
                 printf("Echec de l'operation.\n");
             }
         } break;
 
         case 6: {
             // Inserer un relevé
             int id;
             float val;
             printf("Entrez l'ID du capteur: ");
             scanf("%d", &id);
             printf("Entrez la valeur mesuree (ppm): ");
             scanf("%f", &val);
             int readingID = insertReadingDB(id, val);
             if (readingID > 0) {
                 printf("Nouveau relevé inséré (ID=%d).\n", readingID);
             } else {
                 printf("Echec de l'insertion.\n");
             }
         } break;
 
         case 0:
             // Quitter
             break;
 
         default:
             printf("Choix invalide.\n");
     }
 }