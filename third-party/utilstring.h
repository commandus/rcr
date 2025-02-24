#ifndef UTILSTRING_H_
#define UTILSTRING_H_

#include <string>

// read file
std::string file2string(const char *filename);

/**
 * @brief Return hex string
 * @param buffer buffer
 * @param size buffer size
 * @return hex string
 */
std::string hexString(const void *buffer, size_t size);

/**
 * @brief Return hex string
 * @param data binary data
 * @return string hex
 */
std::string hexString(const std::string &data);

/**
 * @brief Return binary data string
 * @param hex hex string
 * @return binary data string
 */
std::string hex2string(const std::string &hex);

std::string file2string(const char *filename);
std::string file2string(const std::string &filename);

/**
 * Append string to file
 * @param filename file name
 * @param value string
 */
bool append2file(
	const std::string &filename,
	const std::string &value
);

// write file
bool string2file(const std::string &filename, const std::string &value);

std::string trim(std::string s);

bool isHex(const std::string &value);

std::string replaceAll(std::string str, const std::string& from, const std::string& to);

#endif
