#include "supplies_module.h"

#define FOOD_KEY "food"
#define MEDICINE_KEY "medicine"
#define DB_NAME "../build/database"

void init_rocksdb_supplies()
{

    rocksdb_options_t* opts = rocksdb_options_create();
    rocksdb_options_set_create_if_missing(opts, 1);
    rocksdb_options_set_compression(opts, rocksdb_snappy_compression);
    char* err = NULL;
    rocksdb_t* db = rocksdb_open(opts, DB_NAME, &err);
    if (err != NULL)
    {
        fprintf(stderr, "database open %s\n", err);
        return;
    }
    free(err);
    err = NULL;

    // Check if the keys already exist in the database
    rocksdb_readoptions_t* ro = rocksdb_readoptions_create();
    size_t rlen;
    char* food_supply_json = rocksdb_get(db, ro, FOOD_KEY, strlen(FOOD_KEY), &rlen, &err);
    if (err != NULL)
    {
        fprintf(stderr, "get food key %s\n", err);
        rocksdb_close(db);
        return;
    }
    if (food_supply_json == NULL)
    {
        // Initialize food supply data
        food_supply_json = "{\"meat\": 0, \"vegetables\": 0, \"fruits\": 0, \"water\": 0}";
        rocksdb_writeoptions_t* wo = rocksdb_writeoptions_create();
        rocksdb_put(db, wo, FOOD_KEY, strlen(FOOD_KEY), food_supply_json, strlen(food_supply_json), &err);
        if (err != NULL)
        {
            fprintf(stderr, "put food key %s\n", err);
            rocksdb_close(db);
            return;
        }
    }

    // Check if MEDICINE_KEY already exists in the database
    char* medicine_supply_json = rocksdb_get(db, ro, MEDICINE_KEY, strlen(MEDICINE_KEY), &rlen, &err);
    if (err != NULL)
    {
        fprintf(stderr, "get medicine key %s\n", err);
        rocksdb_close(db);
        return;
    }
    if (medicine_supply_json == NULL)
    {
        // Initialize medicine supply data
        medicine_supply_json = "{\"antibiotics\": 0, \"analgesics\": 0, \"bandages\": 0}";
        rocksdb_writeoptions_t* wo = rocksdb_writeoptions_create();
        rocksdb_put(db, wo, MEDICINE_KEY, strlen(MEDICINE_KEY), medicine_supply_json, strlen(medicine_supply_json),
                    &err);
        if (err != NULL)
        {
            fprintf(stderr, "put medicine key %s\n", err);
            rocksdb_close(db);
            return;
        }
    }

    rocksdb_close(db);
}

FoodSupply* get_food_supply()
{
    rocksdb_options_t* opts = rocksdb_options_create();
    rocksdb_options_set_create_if_missing(opts, 1);
    rocksdb_options_set_compression(opts, rocksdb_snappy_compression);

    char* err = NULL;
    rocksdb_t* db = rocksdb_open_for_read_only(opts, DB_NAME, 0, &err);
    if (err != NULL)
    {
        fprintf(stderr, "database open %s\n", err);
        return NULL;
    }

    rocksdb_readoptions_t* ro = rocksdb_readoptions_create();
    size_t rlen;
    char* food_supply_json = rocksdb_get(db, ro, FOOD_KEY, strlen(FOOD_KEY), &rlen, &err);
    if (err != NULL)
    {
        fprintf(stderr, "get key %s\n", err);
        rocksdb_close(db);
        return NULL;
    }

    rocksdb_close(db);

    FoodSupply* food_supply = (FoodSupply*)malloc(sizeof(FoodSupply));
    cJSON* json = cJSON_Parse(food_supply_json);
    food_supply->meat = cJSON_GetObjectItem(json, "meat")->valueint;
    food_supply->vegetables = cJSON_GetObjectItem(json, "vegetables")->valueint;
    food_supply->fruits = cJSON_GetObjectItem(json, "fruits")->valueint;
    food_supply->water = cJSON_GetObjectItem(json, "water")->valueint;

    cJSON_Delete(json);
    free(food_supply_json);

    return food_supply;
}

MedicineSupply* get_medicine_supply()
{
    rocksdb_options_t* opts = rocksdb_options_create();
    rocksdb_options_set_create_if_missing(opts, 1);
    rocksdb_options_set_compression(opts, rocksdb_snappy_compression);

    char* err = NULL;
    rocksdb_t* db = rocksdb_open_for_read_only(opts, DB_NAME, 0, &err);
    if (err != NULL)
    {
        fprintf(stderr, "database open %s\n", err);
        return NULL;
    }

    rocksdb_readoptions_t* ro = rocksdb_readoptions_create();
    size_t rlen;
    char* medicine_supply_json = rocksdb_get(db, ro, MEDICINE_KEY, strlen(MEDICINE_KEY), &rlen, &err);
    if (err != NULL)
    {
        fprintf(stderr, "get key %s\n", err);
        rocksdb_close(db);
        return NULL;
    }

    rocksdb_close(db);

    MedicineSupply* medicine_supply = (MedicineSupply*)malloc(sizeof(MedicineSupply));
    cJSON* json = cJSON_Parse(medicine_supply_json);
    medicine_supply->antibiotics = cJSON_GetObjectItem(json, "antibiotics")->valueint;
    medicine_supply->analgesics = cJSON_GetObjectItem(json, "analgesics")->valueint;
    medicine_supply->bandages = cJSON_GetObjectItem(json, "bandages")->valueint;

    cJSON_Delete(json);
    free(medicine_supply_json);

    return medicine_supply;
}

void update_supplies_from_json(FoodSupply* food_supply, MedicineSupply* medicine_supply, cJSON* json)
{

    cJSON* food_object = cJSON_GetObjectItem(json, "food");
    if (food_object != NULL && food_object->type == cJSON_Object)
    {
        cJSON* fruits_item = cJSON_GetObjectItem(food_object, "fruits");
        if (fruits_item != NULL && fruits_item->type == cJSON_Number)
        {
            food_supply->fruits = (int)fmax(0, (double)food_supply->fruits + fruits_item->valueint);
        }
        cJSON* vegetables_item = cJSON_GetObjectItem(food_object, "vegetables");
        if (vegetables_item != NULL && vegetables_item->type == cJSON_Number)
        {
            food_supply->vegetables = (int)fmax(0, (double)food_supply->vegetables + vegetables_item->valueint);
        }
        cJSON* meat_item = cJSON_GetObjectItem(food_object, "meat");
        if (meat_item != NULL && meat_item->type == cJSON_Number)
        {
            food_supply->meat = (int)fmax(0, (double)food_supply->meat + meat_item->valueint);
        }
        cJSON* water_item = cJSON_GetObjectItem(food_object, "water");
        if (water_item != NULL && water_item->type == cJSON_Number)
        {
            food_supply->water = (int)fmax(0, (double)food_supply->water + water_item->valueint);
        }
    }

    cJSON* medicine_object = cJSON_GetObjectItem(json, "medicine");
    if (medicine_object != NULL && medicine_object->type == cJSON_Object)
    {
        cJSON* antibiotics_item = cJSON_GetObjectItem(medicine_object, "antibiotics");
        if (antibiotics_item != NULL && antibiotics_item->type == cJSON_Number)
        {
            medicine_supply->antibiotics =
                (int)fmax(0, (double)medicine_supply->antibiotics + antibiotics_item->valueint);
        }
        cJSON* analgesics_item = cJSON_GetObjectItem(medicine_object, "analgesics");
        if (analgesics_item != NULL && analgesics_item->type == cJSON_Number)
        {
            medicine_supply->analgesics = (int)fmax(0, (double)medicine_supply->analgesics + analgesics_item->valueint);
        }
        cJSON* bandages_item = cJSON_GetObjectItem(medicine_object, "bandages");
        if (bandages_item != NULL && bandages_item->type == cJSON_Number)
        {
            medicine_supply->bandages = (int)fmax(0, (double)medicine_supply->bandages + bandages_item->valueint);
        }
    }

    update_food_supply_in_db(food_supply);
    update_medicine_supply_in_db(medicine_supply);
}

void update_food_supply_in_db(FoodSupply* food_supply)
{
    rocksdb_options_t* opts = rocksdb_options_create();
    rocksdb_options_set_create_if_missing(opts, 1);
    rocksdb_options_set_compression(opts, rocksdb_snappy_compression);

    char* err = NULL;
    rocksdb_t* db = rocksdb_open(opts, DB_NAME, &err);
    if (err != NULL)
    {
        fprintf(stderr, "database open %s\n", err);
        free(err);
        return;
    }

    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "meat", food_supply->meat);
    cJSON_AddNumberToObject(json, "vegetables", food_supply->vegetables);
    cJSON_AddNumberToObject(json, "fruits", food_supply->fruits);
    cJSON_AddNumberToObject(json, "water", food_supply->water);

    char* food_supply_json = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    rocksdb_writeoptions_t* wo = rocksdb_writeoptions_create();
    rocksdb_put(db, wo, FOOD_KEY, strlen(FOOD_KEY), food_supply_json, strlen(food_supply_json), &err);
    if (err != NULL)
    {
        fprintf(stderr, "put key %s\n", err);
        free(err);
    }

    free(food_supply_json);
    rocksdb_close(db);
}

void update_medicine_supply_in_db(MedicineSupply* medicine_supply)
{
    rocksdb_options_t* opts = rocksdb_options_create();
    rocksdb_options_set_create_if_missing(opts, 1);
    rocksdb_options_set_compression(opts, rocksdb_snappy_compression);

    char* err = NULL;
    rocksdb_t* db = rocksdb_open(opts, DB_NAME, &err);
    if (err != NULL)
    {
        fprintf(stderr, "database open %s\n", err);
        free(err);
        return;
    }

    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "antibiotics", medicine_supply->antibiotics);
    cJSON_AddNumberToObject(json, "analgesics", medicine_supply->analgesics);
    cJSON_AddNumberToObject(json, "bandages", medicine_supply->bandages);

    char* medicine_supply_json = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    rocksdb_writeoptions_t* wo = rocksdb_writeoptions_create();
    rocksdb_put(db, wo, MEDICINE_KEY, strlen(MEDICINE_KEY), medicine_supply_json, strlen(medicine_supply_json), &err);
    if (err != NULL)
    {
        fprintf(stderr, "put key %s\n", err);
        free(err);
    }

    free(medicine_supply_json);
    rocksdb_close(db);
}
