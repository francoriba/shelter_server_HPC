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

#include "myRocksDbWrapper.hpp"

RocksDbWrapper::RocksDbWrapper(const std::string &pathDatabase)
{
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, pathDatabase, &m_database);
    if (!status.ok())
    {
        throw std::runtime_error("Failed to open/create database due: " + status.ToString());
    }
}

RocksDbWrapper::~RocksDbWrapper() {
    m_database->Close();
    delete m_database;
}

void RocksDbWrapper::put(const std::string &key, const rocksdb::Slice &value)
{
    rocksdb::Status status = m_database->Put(rocksdb::WriteOptions(), key, value);
    if (!status.ok())
    {
        throw std::runtime_error("Failed to put key-value pair into database due: " + status.ToString());
    }
}

bool RocksDbWrapper::get(const std::string &key, std::string &value)
{
    rocksdb::Status status = m_database->Get(rocksdb::ReadOptions(), key, &value);
    if (status.IsNotFound())
    {
        return false;
    }
    else if (!status.ok())
    {
        throw std::runtime_error("Failed to get value from database due: " + status.ToString());
    }
    return true;
}

void RocksDbWrapper::delete_(const std::string &key)
{
    rocksdb::Status status = m_database->Delete(rocksdb::WriteOptions(), key);
    if (!status.ok())
    {
        throw std::runtime_error("Failed to delete key from database due: " + status.ToString());
    }
}

std::string RocksDbWrapper::getValueByKey(const std::string &key)
{
    std::string value;
    rocksdb::Status status = m_database->Get(rocksdb::ReadOptions(), key, &value);
    if (status.IsNotFound())
    {
        throw std::runtime_error("Key not found in the database");
    }
    else if (!status.ok())
    {
        throw std::runtime_error("Failed to get value from database due: " + status.ToString());
    }
    return value;
}

int RocksDbWrapper::countOccurrences(const std::string& targetValue)
{

    int count = 0;
    rocksdb::ReadOptions options;
    rocksdb::Iterator* it = m_database->NewIterator(options);

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string value = it->value().ToString();
        if (value.find(targetValue) != std::string::npos) {
            count++;
        }
    }
    if (!it->status().ok()) {
        std::cerr << "Iterator failed: " << it->status().ToString() << std::endl;
    }

    delete it;

    return count;
}

std::vector<json> RocksDbWrapper::getJsonByKeySubstrings(const std::vector<std::string>& substrings) {
    std::vector<json> results;

    rocksdb::ReadOptions options;
    rocksdb::Iterator* it = m_database->NewIterator(options);

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string key = it->key().ToString();
        std::string value = it->value().ToString();
        bool all_substrings_found = true;

        // Verify that all substrings are present in the key
        for (const std::string& substr : substrings) {
            if (key.find(substr) == std::string::npos) {
                all_substrings_found = false;
                break;
            }
        }

        // If all substrings are found, add the key-value pair to the results
        if (all_substrings_found) {
            json result;
            result[key] = value;
            results.push_back(result);
        }
    }

    if (!it->status().ok()) {
        std::cerr << "Iterator failed: " << it->status().ToString() << std::endl;
    }

    delete it;

    return results;
}
