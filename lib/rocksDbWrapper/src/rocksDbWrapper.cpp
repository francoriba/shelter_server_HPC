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

#include <rocksDbWrapper.hpp>

RocksDbWrapper::RocksDbWrapper(const std::string& pathDatabase)
{
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::DB* dbPtr = nullptr; // Declare a raw pointer
    rocksdb::Status status = rocksdb::DB::Open(options, pathDatabase, &dbPtr);

    if (!status.ok())
    {
        throw std::runtime_error("Failed to open/create database due: " + status.ToString());
    }

    // Wrap the raw pointer in a smart pointer
    m_database.reset(dbPtr);
}

void RocksDbWrapper::put(const std::string& key, const rocksdb::Slice& value)
{
    rocksdb::Status status = m_database->Put(rocksdb::WriteOptions(), key, value);
    if (!status.ok())
    {
        throw std::runtime_error("Failed to put key-value pair: " + status.ToString());
    }
}

bool RocksDbWrapper::get(const std::string& key, std::string& value)
{
    rocksdb::Status status = m_database->Get(rocksdb::ReadOptions(), key, &value);

    if (status.IsNotFound())
    {
        return false;
    }

    if (!status.ok())
    {
        throw std::runtime_error("Failed to get key-value pair: " + status.ToString());
    }

    return true;
}

void RocksDbWrapper::delete_(const std::string& key)
{
    rocksdb::Status status = m_database->Delete(rocksdb::WriteOptions(), key);

    if (!status.ok())
    {
        throw std::runtime_error("Failed to delete key: " + status.ToString());
    }
}
