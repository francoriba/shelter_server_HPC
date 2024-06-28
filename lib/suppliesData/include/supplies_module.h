#pragma once

#include "../lib/cJSON/include/cJSON.h"
#include <errno.h>
#include <math.h>
#include <rocksdb/c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct
{
    int meat;
    int vegetables;
    int fruits;
    int water;
} FoodSupply;

typedef struct
{
    int antibiotics;
    int analgesics;
    int bandages;
} MedicineSupply;

/**
 * @brief Initializes the supplies database and checks if the keys for food and medicine exist.
 * If not, initializes them with default values.
 */
void init_rocksdb_supplies();

/**
 * @brief Retrieves the current food supply from the database.
 *
 * @return A pointer to the FoodSupply struct containing the current food supply data.
 *         Returns NULL if there was an error or if the data is not found.
 */
FoodSupply* get_food_supply();

/**
 * @brief Retrieves the current medicine supply from the database.
 *
 * @return A pointer to the MedicineSupply struct containing the current medicine supply data.
 *         Returns NULL if there was an error or if the data is not found.
 */
MedicineSupply* get_medicine_supply();

/**
 * @brief Updates the supplies data based on the provided JSON object.
 *        This function updates both food and medicine supplies.
 *
 * @param food_supply     Pointer to the FoodSupply struct to be updated.
 * @param medicine_supply Pointer to the MedicineSupply struct to be updated.
 * @param json            cJSON object containing the new supplies data.
 */
void update_supplies_from_json(FoodSupply* food_supply, MedicineSupply* medicine_supply, cJSON* json);

/**
 * @brief Updates the food supply data in the database.
 *
 * @param food_supply Pointer to the FoodSupply struct containing the updated data.
 */
void update_food_supply_in_db(FoodSupply* food_supply);

/**
 * @brief Updates the medicine supply data in the database.
 *
 * @param medicine_supply Pointer to the MedicineSupply struct containing the updated data.
 */
void update_medicine_supply_in_db(MedicineSupply* medicine_supply);
