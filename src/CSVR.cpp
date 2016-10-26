#include "CSVReader.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace klok {
namespace data {

static const char* COL_PROD_CODE = "Product code";
static const char* COL_PROD_NAME = "Product Name";
static const char* COL_PROD_SALES_RATE = "Sales Rate";
static const char* COL_PROD_STOCK_QTY = "Stock Quantity";

struct FileIO {
  enum Error {
    FileReadOK = 0,
    FileNotReadable,
    InvalidNumberOfCommas,
    SeperatorLineNotFound,
    UnexpectedFileEnd,
    NotEnoughColumns,
    InvalidColumnNames,
    ItemCommaError,
    ItemInvalidData
  };
};
// struct ProductStock
// {
// 	std::string Code,SalesRate,Name,ShortName,StockQuantity;
// 	static std::int32_t fromFile(const char * fileName,std::string &
// error_desc,std::vector<ProductStock> & outStockData);
// };

using std::cin;
using std::cout;

static const char* const ws = " \t\n\r\f\v";

// trim from end of string (right)
inline std::string& rtrim(std::string& s, const char* t = ws) {
  s.erase(s.find_last_not_of(t) + 1);
  return s;
}

// trim from beginning of string (left)
inline std::string& ltrim(std::string& s, const char* t = ws) {
  s.erase(0, s.find_first_not_of(t));
  return s;
}

// trim from both ends of string (left & right)
inline std::string& trim(std::string& s, const char* t = ws) {
  return ltrim(rtrim(s, t), t);
}

size_t split(const std::string& text,
             char sep,
             std::vector<std::string>& tokens) {
  std::size_t start = 0, end = 0;
  while ((end = text.find(sep, start)) != std::string::npos) {
    if (end != start) {
      tokens.push_back(text.substr(start, end - start));
    }
    start = end + 1;
  }
  if (end != start) {
    tokens.push_back(text.substr(start));
  }
  return tokens.size();
}

int32_t ProductStock::fromFile(const char* fileName,
                                    std::string& error_desc,
                                    std::vector<ProductStock>& outStockData) {
  std::ifstream infile(fileName);
  std::string line;
  if (!infile) {
    error_desc = "Could Not Open File : ";
    error_desc += fileName;
    return FileIO::FileNotReadable;
  }

  std::getline(infile, line);

  cout << "Line  : " << line << '\n';

  const int commas = std::count(line.begin(), line.end(), ',');

  if (commas != 3) {
    error_desc = "Invalid # of Commas";
    return FileIO::InvalidNumberOfCommas;
  }

  std::vector<std::string> column_titles;

  if (split(line, ',', column_titles) != 4) {
    error_desc = "Not Enough/ More Columns";
    return FileIO::InvalidNumberOfCommas;
  }

  int prodCodeIndex = -1;
  int prodNameIndex = -1;
  int prodSalesRateIndex = -1;
  int prodStockQtyIndex = -1;

  for (size_t i = 0; i < column_titles.size(); ++i) {
    // std::cout << ':' << COL_PROD_STOCK_QTY << ':'
    //           << column_titles[i][column_titles[i].size() - 1] << ':' << '\n';
    if (trim(column_titles[i]) == COL_PROD_NAME) {
      prodNameIndex = i;
    } else if (trim(column_titles[i]) == COL_PROD_CODE) {
      prodCodeIndex = i;
    } else if (trim(column_titles[i]) == COL_PROD_STOCK_QTY) {
      prodStockQtyIndex = i;
    } else if (trim(column_titles[i]) == COL_PROD_SALES_RATE) {
      prodSalesRateIndex = i;
    }
  }

  if (-1 == prodCodeIndex || -1 == prodNameIndex || -1 == prodSalesRateIndex ||
      -1 == prodStockQtyIndex) {
    std::cout << prodCodeIndex << prodNameIndex << prodSalesRateIndex
              << prodStockQtyIndex << '\n';
    error_desc = "Invalid column names";
    return FileIO::InvalidColumnNames;
  }

  if (infile.eof() || !infile) {
    error_desc = "Unexpected file end ";
    error_desc += fileName;
    return FileIO::UnexpectedFileEnd;
  }

  std::getline(infile, line);

  const int count_equals = std::count(line.begin(), line.end(), '=');

  if (count_equals <= 0) {
    error_desc = "2nd line error";
    return FileIO::SeperatorLineNotFound;
  }

  if (infile.eof() || !infile) {
    error_desc = "Unexpected file end ";
    error_desc += fileName;
    return FileIO::UnexpectedFileEnd;
  }

  int itemLine = 0;
  while (!infile.eof()) {
    ++itemLine;

    std::getline(infile, line);

    if (1 > line.size())
      continue;

    const int data_commas = std::count(line.begin(), line.end(), ',');
    if (3 != data_commas) {
      error_desc = "Item comma error";
      return FileIO::ItemCommaError;
    }

    std::vector<std::string> itemData;
    const size_t foundFieldsCount = split(line, ',', itemData);

    if (4 != foundFieldsCount) {
      error_desc = "Item comma error";
      return FileIO::ItemCommaError;
    }

    if (1 > itemData[prodCodeIndex].size()) {
      error_desc = "Item data len error:Code";
      return FileIO::ItemInvalidData;
    }
    if (1 > itemData[prodNameIndex].size()) {
      error_desc = "Item data len error:Name";
      return FileIO::ItemInvalidData;
    }
    if (1 > itemData[prodSalesRateIndex].size()) {
      error_desc = "Item data len error:SalesRate";
      return FileIO::ItemInvalidData;
    }
    if (1 > itemData[prodStockQtyIndex].size()) {
      error_desc = "Item data len error:Quantity";
      return FileIO::ItemInvalidData;
    }

    ProductStock toInsert;
    toInsert.Code = itemData[prodCodeIndex];
    toInsert.SalesRate = itemData[prodSalesRateIndex];
    toInsert.Name = itemData[prodNameIndex];
    toInsert.ShortName = itemData[prodNameIndex].substr(0, 10);
    toInsert.StockQuantity = itemData[prodStockQtyIndex];

    outStockData.push_back(toInsert);
  }

  return FileIO::FileReadOK;
}

// struct UserData
// {
// 	std::string Code,Name,Password;
// 	static std::int32_t fromFile(const char * fileName,std::string &
// error_desc,std::vector<UserData> & outUserData);
// };

static const char* COL_USER_CODE = "Code";
static const char* COL_USER_NAME = "Name";
static const char* COL_USER_PASSWORD = "Password";

int32_t UserData::fromFile(const char* fileName,
                                std::string& error_desc,
                                std::vector<UserData>& outUserData) {
  std::ifstream infile(fileName);
  std::string line;
  if (!infile) {
    error_desc = "Could Not Open File : ";
    error_desc += fileName;
    return FileIO::FileNotReadable;
  }

  std::getline(infile, line);

  cout << "Line  : " << line << '\n';

  const int commas = std::count(line.begin(), line.end(), ',');

  if (2 != commas) {
    error_desc = "Invalid # of Commas";
    return FileIO::InvalidNumberOfCommas;
  }

  std::vector<std::string> column_titles;

  if (3 != split(line, ',', column_titles)) {
    error_desc = "Not Enough/ More Columns";
    return FileIO::InvalidNumberOfCommas;
  }

  int userCodeIndex = -1;
  int userNameIndex = -1;
  int userPasswordIndex = -1;

  for (size_t i = 0; i < column_titles.size(); ++i) {
    if (trim(column_titles[i]) == COL_USER_NAME) {
      userNameIndex = i;
    } else if (trim(column_titles[i]) == COL_USER_CODE) {
      userCodeIndex = i;
    } else if (trim(column_titles[i]) == COL_USER_PASSWORD) {
      userPasswordIndex = i;
    }
  }

  if (-1 == userCodeIndex || -1 == userNameIndex || -1 == userPasswordIndex) {
    error_desc = "Invalid column names";
    return FileIO::InvalidColumnNames;
  }

  if (infile.eof() || !infile) {
    error_desc = "Unexpected file end ";
    error_desc += fileName;
    return FileIO::UnexpectedFileEnd;
  }

  int itemLine = 0;
  while (!infile.eof()) {
    ++itemLine;

    std::getline(infile, line);

    if (1 > line.size())
      continue;

    const int data_commas = std::count(line.begin(), line.end(), ',');
    if (2 != data_commas) {
      error_desc = "Item comma error";
      return FileIO::ItemCommaError;
    }

    std::vector<std::string> itemData;
    const size_t foundFieldsCount = split(line, ',', itemData);

    if (3 != foundFieldsCount) {
      error_desc = "Item comma error";
      return FileIO::ItemCommaError;
    }

    if (1 > itemData[userCodeIndex].size()) {
      error_desc = "Item data len error:Code";
      return FileIO::ItemInvalidData;
    }
    if (1 > itemData[userNameIndex].size()) {
      error_desc = "Item data len error:Name";
      return FileIO::ItemInvalidData;
    }
    if (1 > itemData[userPasswordIndex].size()) {
      error_desc = "Item data len error:Password";
      return FileIO::ItemInvalidData;
    }

    UserData dat;

    dat.Code = itemData[userCodeIndex];
    dat.Name = itemData[userNameIndex];
    dat.Password = itemData[userPasswordIndex];

    outUserData.push_back(dat);
  }

  return FileIO::FileReadOK;
}
}
}

