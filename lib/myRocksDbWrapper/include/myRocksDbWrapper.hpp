/*
 * DatabaseManagment - RocksDbWrapper
 * Copyright (C) 2024, Operating Systems II.
 * Apr 20, 2024.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#ifndef _ROCKS_DB_WRAPPER_HPP
#define _ROCKS_DB_WRAPPER_HPP

#include <rocksdb/db.h>
#include <memory>
#include <string>
#include <iostream>
#include "../build/_deps/nlohmann_json-src/single_include/nlohmann/json.hpp"

using json = nlohmann::json;


class RocksDbWrapper
{
public:
    /**
     * @brief Constructor.
     * @param pathDatabase Path to the database.
     */
    explicit RocksDbWrapper(const std::string &pathDatabase);

    ~RocksDbWrapper(); // Destructor

    /**
     * @brief Put a key-value pair in the database.
     * @param key Key to put.
     * @param value Value to put.
     *
     * @note If the key already exists, the value will be overwritten.
     */
    void put(const std::string &key, const rocksdb::Slice &value);

    /**
     * @brief Get a value from the database.
     *
     * @param key Key to get.
     * @param value Value to get (rocksdb::PinnableSlice).
     *
     * @return bool True if the operation was successful.
     * @return bool False if the key was not found.
     */
    bool get(const std::string &key, std::string &value);

    /**
     * @brief Delete a key-value pair from the database.
     *
     * @param key Key to delete.
     */
    void delete_(const std::string &key);

    /**
     * @brief Get a pointer to the RocksDB database instance.
     *
     * This method returns a pointer to the RocksDB database instance managed by this wrapper.
     *
     * @return A pointer to the RocksDB database instance.
     */
    rocksdb::DB* getDatabase() const { return m_database; };

    /**
     * @brief Retrieve the value associated with a given key.
     *
     * This method retrieves the value associated with the specified key from the database.
     *
     * @param key The key for which to retrieve the value.
     * @return The value associated with the provided key.
     *
     * @throws std::runtime_error if the key is not found in the database, or if there is an error retrieving the value.
     */
    std::string getValueByKey(const std::string &key);

    /**
     * @brief Prints all key-value pairs stored in the RocksDB database.
     *
     * This function iterates over all key-value pairs in the database and prints each key-value pair to the standard output. Should only be used for debugging purposes.
     *
     * @return void
     */
    void printAllValues();

    /**
     * @brief Counts the occurrences of a specified string in the values stored in the RocksDB database.
     *
     * This function iterates over all key-value pairs in the database and counts the number of occurrences of the specified string in the values.
     *
     * @param targetValue The string to search for within the values.
     * @return The number of occurrences of the specified string in the values.
    */
    int countOccurrences(const std::string& targetValue);

    /**
     * @brief Retrieves JSON data from the RocksDB database based on multiple substrings in the keys.
     * 
     * @param substrings A vector of substrings to search for in the keys.
     * @return A vector of JSON objects containing key-value pairs that match all substrings.
     * 
     * This function retrieves JSON data from the RocksDB database by iterating through the key-value pairs.
     * It searches for keys that contain all the provided substrings and constructs JSON objects
     * with matching key-value pairs. The resulting vector contains all JSON objects that meet the criteria.
     */
    std::vector<json> getJsonByKeySubstrings(const std::vector<std::string>& substrings);
    
private:
    rocksdb::DB* m_database;  ///< Database instance.
};

#endif // _ROCKS_DB_WRAPPER_HPP
