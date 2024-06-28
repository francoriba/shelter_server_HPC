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

#include <memory>
#include <rocksdb/db.h>
#include <string>

/**
 * @brief Wrapper class for RocksDB.
 */
class RocksDbWrapper
{
public:
    /**
     * @brief Constructor.
     * @param pathDatabase Path to the database.
     */
    explicit RocksDbWrapper(const std::string& pathDatabase);

    /**
     * @brief Put a key-value pair in the database.
     * @param key Key to put.
     * @param value Value to put.
     *
     * @note If the key already exists, the value will be overwritten.
     */
    void put(const std::string& key, const rocksdb::Slice& value);

    /**
     * @brief Get a value from the database.
     *
     * @param key Key to get.
     * @param value Value to get (rocksdb::PinnableSlice).
     *
     * @return bool True if the operation was successful.
     * @return bool False if the key was not found.
     */
    bool get(const std::string& key, std::string& value);

    /**
     * @brief Delete a key-value pair from the database.
     *
     * @param key Key to delete.
     */
    void delete_(const std::string& key);

private:
    std::shared_ptr<rocksdb::DB> m_database; ///< Database instance.
};

#endif // _ROCKS_DB_WRAPPER_HPP
