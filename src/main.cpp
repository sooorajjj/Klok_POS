
#include <SQLiteCpp/SQLiteCpp.h>
#include <algorithm>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <sstream>
#include <stdlib.h>

#include "Config.hpp"
#include "PosDataStructures.hpp"
#include "Visiontek.hpp"
#include "CSVReader.hpp"

extern "C" {
#include <0202lcd.h>
#include <V91magswipe.h>
#include <X6x8.h>
#include <header.h>
#include <printer.h>
}

struct Operation {
  enum Mode { NotSelected = 0, PayCollection, SpotBilling };
};

namespace {

std::string gUserId = "", gUserName = "", gTransId = "", gCustomerId = "",
            gCustomerName = "", gCustomerBalance = "", gCustomerContact = "",
            gCompanyName = "", gCompanyAddress = "", gProductName = "",
            gDeviceId = "F";

SQLite::Database* gDatabasePtr = NULL;

float gBillAmt = 0.00;

std::map<std::string, klok::pc::POSBillEntry> gBillData;
std::map<std::string, klok::pc::Product> gAllSelectedProducts;

typedef std::map<std::string, klok::pc::POSBillEntry>::iterator BillIter_t;

Operation::Mode gMode = Operation::NotSelected;
}
template <typename T>
std::string tostr(const T& t) {
  std::ostringstream os;
  os << t;
  return os.str();
}
static void appendToIfFound(std::string& buffer,
                            klok::pos::Configuration::Data_t& data,
                            const char* key) {
  klok::pos::Configuration::Data_t::iterator it = data.find(key);
  if (it != data.end())
    buffer.append(it->second + '\n');
}

static void addItemToBill(const klok::pc::Product& product, float quantity) {
  if (gBillData.count(product.id) > 0) {
    printf("The bill already contains item %s - %s  : %f , changing to %f\n",
           product.short_name.c_str(), product.code.c_str(),
           gBillData[product.id].Quantity, quantity);

    if (quantity == 0)
      gBillData.erase(product.id);
    else
      gBillData[product.id].Quantity = quantity;

  } else {
    float salesRateParsed = 0.0f;

    if (std::sscanf(product.sales_rate.c_str(), "%f", &salesRateParsed) == 1) {
      klok::pc::POSBillEntry newEntry;
      newEntry.Quantity = quantity;
      newEntry.SalesRate = salesRateParsed;
      gBillData[product.id] = newEntry;

      printf("inserted new item to bill \n");
    } else {
      printf("parsing price for item failed %s - %s\n",
             product.short_name.c_str(), product.sales_rate.c_str());
    }
  }
}

void prompt_shutdown() {
  lcd::DisplayText(3, 0, "press power to shutdown", 1);
  while (true)
    ;
}

void display_fatal_error(const char* userError, std::exception& exc) {
  // call DisplayText
  lk_dispclr();
  lcd::DisplayText(1, 0, userError, 1);

  // Display exc.what()
  std::string err = exc.what();
  lcd::DisplayText(2, 0, err.c_str(), 1);
}

static void closeDatabase() {
  if (gDatabasePtr != NULL) {
    delete gDatabasePtr;
    gDatabasePtr = NULL;
  }
}

static SQLite::Database& getDatabase() {
  if (gDatabasePtr == NULL) {
    try {
      gDatabasePtr =
          new SQLite::Database("/mnt/jffs2/PayCollect.db",
                               SQLite::OPEN_READWRITE, SQLite::OPEN_CREATE);
    } catch (std::exception& e) {
      display_fatal_error("Database Open Failed", e);
      prompt_shutdown();
    }
  } else {
    return *gDatabasePtr;
  }
}

const char* getPosCustomerDisplayName(const klok::pc::Customer& inCustomer) {
  return inCustomer.id.c_str();
}
const char* getPosProductDisplayName(const klok::pc::Product& inProduct) {
  std::string product_name_list = inProduct.code + " :" + inProduct.name;

  return product_name_list.c_str();
}

const char* getPosTransactionDisplayName(
    const klok::pc::Transaction& inTransaction) {
  return inTransaction.trans_id.c_str();
}

const char* getPosBillDisplayName(
    const klok::pc::PosBillHeader& inPosBillHeader) {
  const std::string dateTimeTrimmed = inPosBillHeader.date_time.substr(5);
  std::string bill_list = inPosBillHeader.id + " : " +
                          dateTimeTrimmed.substr(0, dateTimeTrimmed.size() - 3);
  return bill_list.c_str();
}

const char* getPosTransactionDatesDisplayName(const std::string& inDate) {
  return inDate.c_str();
}

std::string getCurrentTime() {
  time_t now = time(0);
  tm* ltm = localtime(&now);

  char buffer[50] = {0};
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 1900 + ltm->tm_year,
          1 + ltm->tm_mon, ltm->tm_mday, 1 + ltm->tm_hour, 1 + ltm->tm_min,
          1 + ltm->tm_sec);

  return buffer;
}

int returncheck(int r) {
  switch (r) {
    case -1:
      lk_dispclr();
      lcd::DisplayText(2, 5, "device not opened", 1);
      break;

    case -2:
      lk_dispclr();
      lcd::DisplayText(2, 5, "length error", 1);
      break;

    case -3:
      lk_dispclr();
      lcd::DisplayText(2, 5, "NO Paper", 1);
      break;

    case -4:
      lk_dispclr();
      lcd::DisplayText(2, 5, "Low Battery", 1);
      break;

    case -5:
      lk_dispclr();
      lcd::DisplayText(2, 5, "Max temp", 1);
      break;

    case -6:
      lk_dispclr();
      lcd::DisplayText(2, 5, "No Lines", 1);
      break;
    case -7:
      lk_dispclr();
      lcd::DisplayText(2, 5, "WRITE_ERROR", 1);
      break;

    case 0:
      return 0;
  }

  return -1;
}

void insertAndPrint(std::string principleAmtString,
                    std::string addLessString,
                    std::string netAmtString,
                    std::string customerId,
                    std::string userId,
                    std::string transId,
                    std::string customerBalance) {
  klok::pc::Transaction toInsert;
  toInsert.cust_id = customerId;
  toInsert.user_id = userId;
  toInsert.gross_amt = principleAmtString;
  toInsert.add_less = addLessString;
  toInsert.net_amt = netAmtString;
  toInsert.date_time = getCurrentTime();

  if (klok::pc::Transaction::InsertIntoTable(getDatabase(), toInsert) == 0) {
    std::string buff, buff1, buff2, buff3, buff4, buff5;

    int x = lk_getkey();

    if (x == klok::pc::KEYS::KEY_ENTER) {
      prn_open();
      if (prn_paperstatus() != 0) {
        lk_dispclr();
        lcd::DisplayText(3, 5, "No Paper !", 1);
        lk_getkey();
        return;
      }

      klok::pos::Configuration c;
      klok::pos::Configuration::ParseFromFile("/mnt/jffs2/POS.cfg", c);

      appendToIfFound(buff, c.getData(), "Company_Title_L1");
      appendToIfFound(buff, c.getData(), "Company_Title_L2");
      appendToIfFound(buff1, c.getData(), "Company_Addr_L1");
      appendToIfFound(buff1, c.getData(), "Company_Addr_L2");
      appendToIfFound(buff1, c.getData(), "Company_Addr_L3");
      appendToIfFound(buff1, c.getData(), "Company_Contact_L1");
      appendToIfFound(buff1, c.getData(), "Company_Contact_L2");
      appendToIfFound(buff1, c.getData(), "Company_Email_L1");
      appendToIfFound(buff2, c.getData(), "Bill_name_1");
      // appendToIfFound(buff2,c.getData(),"Bill_name_2");
      // appendToIfFound(buff2,c.getData(),"Bill_name_3");
      // appendToIfFound(buff2,c.getData(),"Report_name_1");
      // appendToIfFound(buff2,c.getData(),"Report_name_2");
      // appendToIfFound(buff2,c.getData(),"Report_name_3");
      // appendToIfFound(buff2,c.getData(),"Report_name_4");
      // appendToIfFound(buff5,c.getData(),"Print_Footer_L1");
      // appendToIfFound(buff5,c.getData(),"Print_Footer_L2");
      // appendToIfFound(buff5,c.getData(),"Print_Footer_L3");
      // appendToIfFound(buff5,c.getData(),"Print_Footer_L4");

      buff1.append("\n");
      buff3.append("     Bill No             ");
      buff3.append(transId);
      buff3.append("\n");
      buff3.append("     Name                ");
      buff3.append(gCustomerName);
      buff3.append("\n");
      buff3.append("     Contact             ");
      buff3.append(gCustomerContact);
      buff3.append("\n");
      buff3.append("     Gross Amount        ");
      buff3.append(principleAmtString);
      buff3.append("\n");
      buff3.append("     Add/Less            ");
      buff3.append(addLessString);
      buff3.append("\n");
      buff3.append("     -------------------------------\n");
      buff4.append("  CASH       ");
      buff4.append(netAmtString);
      buff4.append("\n");
      buff5.append("     Balance             ");
      buff5.append(customerBalance);
      buff5.append("\n");
      buff5.append("     Billing Username    ");
      buff5.append(gUserName);
      buff5.append("\n");
      buff5.append("     -------------------------------\n");
      appendToIfFound(buff5, c.getData(), "Print_Footer_L1");
      appendToIfFound(buff5, c.getData(), "Print_Footer_L2");
      appendToIfFound(buff5, c.getData(), "Print_Footer_L3");
      appendToIfFound(buff5, c.getData(), "Print_Footer_L4");
      buff5.append("           ");
      buff5.append(getCurrentTime());
      buff5.append("\n");

      lk_dispclr();
      lcd::DisplayText(3, 5, "PRINTING BILL", 1);

      int ret;

      ret = printer::WriteText(buff.c_str(), buff.size(), 2);
      returncheck(ret);

      ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
      returncheck(ret);

      ret = printer::WriteText(buff2.c_str(), buff2.size(), 2);
      returncheck(ret);

      ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
      returncheck(ret);

      ret = printer::WriteText(buff4.c_str(), buff4.size(), 2);
      returncheck(ret);

      ret = printer::WriteText(buff5.c_str(), buff5.size(), 1);
      returncheck(ret);

      ret = printer::WriteText("\n\n\n", 3, 1);
      returncheck(ret);
      ret = prn_paper_feed(1);
      prn_close();

      if (ret == -3) {
        printf("out of the paper");
      } else {
        return;
      }
    } else if (x == klok::pc::KEYS::KEY_CANCEL) {
      return;
    }
  } else {
    printf("failed to InsertIntoTable\n");
  }
}

void updateCustomerBalance(float netAmt,
                           std::string principleAmtString,
                           std::string addLessString,
                           std::string netAmtString,
                           std::string gCustomerBalance) {
  klok::pc::Customer toUpdate;

  int scanned = atoi(gCustomerBalance.c_str());
  float currentAmt = scanned + netAmt;

  char customerBalance[10] = {0};
  sprintf(customerBalance, "%0.2f", currentAmt);

  toUpdate.id = gCustomerId;
  toUpdate.cur_amt = customerBalance;

  std::string display_customerBalance = " Balance    : ";
  display_customerBalance += gCustomerBalance;
  std::string display_principleAmt = " Gross Amt  : ";
  display_principleAmt += principleAmtString;
  std::string display_addLess = " Add/Less   :  ";
  display_addLess += addLessString;
  std::string display_updatedBalance = "Collection  : ";
  display_updatedBalance += customerBalance;

  lk_dispclr();
  lcd::DisplayText(0, 0, display_customerBalance.c_str(), 0);
  lcd::DisplayText(1, 0, display_principleAmt.c_str(), 0);
  lcd::DisplayText(2, 0, display_addLess.c_str(), 0);
  lcd::DisplayText(3, 0, "--------------------", 0);
  lcd::DisplayText(4, 0, display_updatedBalance.c_str(), 0);
  lcd::DisplayText(5, 0, "Press Enter to print", 0);

  insertAndPrint(principleAmtString, addLessString, netAmtString, gCustomerId,
                 gUserId, gTransId, customerBalance);

  if (klok::pc::Customer::UpdateCustomerBalance(
          getDatabase(), gCustomerId.c_str(), toUpdate) == 0)

  {
    printf("Getting selected cust_id for user %s is \n", gCustomerId.c_str());
    return;

  } else {
    printf("CustomerId  %s is not available\n", gCustomerId.c_str());
    return;
  }
}

void net_amt(float principleAmt, float addLess) {
  float netAmt = principleAmt + addLess;
  char principleAmtString[30] = {0};
  char addLessString[30] = {0};
  char netAmtString[30] = {0};

  sprintf(principleAmtString, "%0.2f", principleAmt);
  sprintf(addLessString, "%0.2f", addLess);
  sprintf(netAmtString, "%0.2f", netAmt);

  updateCustomerBalance(netAmt, principleAmtString, addLessString, netAmtString,
                        gCustomerBalance);
}

void add_less(float principleAmt) {
  lk_dispclr();
  lcd::DisplayText(2, 5, "Add/Less", 1);
  lcd::DisplayText(4, 0, "Press F2 to Add, F3 to Less, ENTER to skip", 0);

  int x = lk_getkey();
  lk_dispclr();

  if (x == klok::pc::KEYS::KEY_F2) {
    int res = 0;
    char addLess[10] = {0};

    lcd::DisplayText(1, 0, "Type in Amount to ADD", 0);
    res = lk_getnumeric(4, 0, (unsigned char*)addLess, 10, strlen(addLess));

    float scanned = 0;

    if (sscanf(addLess, "%f", &scanned) == 1 && res > 0) {
      net_amt(principleAmt, scanned);
    } else {
      lk_dispclr();
      lcd::DisplayText(4, 0, "Enter correct Amt", 0);
      lk_getkey();
    }

  } else if (x == klok::pc::KEYS::KEY_F3) {
    int res = 0;
    char addLess[10] = {0};

    lcd::DisplayText(1, 0, "Type Amount to LESS", 0);
    res = lk_getnumeric(4, 0, (unsigned char*)addLess, 10, strlen(addLess));

    float scanned = 0;

    if (sscanf(addLess, "%f", &scanned) == 1 && res > 0) {
      net_amt(principleAmt, -scanned);
    } else {
      lk_dispclr();
      lcd::DisplayText(4, 0, "Enter correct Amt", 0);
      lk_getkey();
    }
  } else if (x == klok::pc::KEYS::KEY_ENTER) {
    float scanned = 0;
    net_amt(principleAmt, scanned);
  }
}

void display_customer_details(const klok::pc::Customer& inCustomer) {
  lk_dispclr();

  std::string Cust_Name = "Name :" + inCustomer.name;
  lcd::DisplayText(0, 0, Cust_Name.c_str(), 0);
  printf("%s\n", Cust_Name.c_str());

  std::string Cust_Bal = "Balance Amt:" + inCustomer.cur_amt;
  lcd::DisplayText(1, 0, Cust_Bal.c_str(), 0);
  printf("%s\n", Cust_Bal.c_str());

  gCustomerName = inCustomer.name;
  gCustomerBalance = inCustomer.cur_amt;
  gCustomerContact = inCustomer.contact;

  int res = 0;
  char grossAmt[10] = {0};

  lcd::DisplayText(3, 0, "Gross Amount", 0);
  res = lk_getnumeric(4, 0, (unsigned char*)grossAmt, 10, strlen(grossAmt));
  float scanned = 0;

  if (sscanf(grossAmt, "%f", &scanned) == 1 && res > 0) {
    add_less(scanned);
  } else {
    lk_dispclr();
    lcd::DisplayText(4, 0, "Enter correct Amt", 0);
    lk_getkey();
  }
}

void getCustomerDetails(std::string display_transid) {
  std::vector<klok::pc::Customer> allCustomers;
  if (klok::pc::Customer::GetAllFromDatabase(getDatabase(), allCustomers, 10) ==
      0) {
    for (int i = 0; i != allCustomers.size(); i++) {
      printf("CustomerId :%s\n", allCustomers[i].id.c_str());
      printf("CustomerName :%s\n", allCustomers[i].name.c_str());
    }

    klok::pc::MenuResult res;
    res.wasCancelled = false;
    res.selectedIndex = -1;

    klok::pc::display_sub_range_with_title(allCustomers,
                                           display_transid.c_str(), 4, res,
                                           &getPosCustomerDisplayName);

    if (!res.wasCancelled) {
      gCustomerId = allCustomers[res.selectedIndex].id;
      display_customer_details(allCustomers[res.selectedIndex]);
    }
  } else {
    printf("failed to GetAllFromDatabase -> getCustomerDetails \n");
  }
}

void PayCollection() {
  printf("PayCollection Activity\n");

  std::string Trans_ID = "";
  if (klok::pc::User::GetNextTransactionIDForUser(
          getDatabase(), gUserId.c_str(), Trans_ID) != 0) {
    printf("Getting Trans_ID for user %s failed \n", gUserId.c_str());
    return;
  } else if (Trans_ID == "") {
    printf("User name  %s is not available\n", gUserId.c_str());
    return;
  }

  gTransId = Trans_ID;

  std::string display_transid = "TransNo :";
  display_transid += Trans_ID;

  printf("Trans_ID :%s\n", Trans_ID.c_str());

  lk_dispclr();
  getCustomerDetails(display_transid);
}

struct BillSummaryDisplayEntry {
  std::string id, code, short_name;
  klok::pc::POSBillEntry details;
};

static const char* BillSummaryDisplayEntryToString(
    const BillSummaryDisplayEntry& entry) {
  char displayBuffer[30] = {0};
  snprintf(displayBuffer, sizeof(displayBuffer) - 1, "%s: %.02f",
           entry.short_name.substr(0, 12).c_str(),
           entry.details.Quantity * entry.details.SalesRate);
  std::string display = displayBuffer;
  return display.c_str();
}

void edit_product_quantity(const BillSummaryDisplayEntry& entry) {
  lk_dispclr();
  int res = 0;

  char qty[10] = {0};
  float scanned = 0;

  lcd::DisplayText(1, 0, entry.short_name.substr(0, 12).c_str(), 0);
  lcd::DisplayText(2, 0, "Enter Quantity", 0);
  res = lk_getnumeric(4, 0, (unsigned char*)qty, 10, strlen(qty));
  if (sscanf(qty, "%f", &scanned) == 1 && res > 0) {
    if (scanned == 0) {
      gBillData.erase(entry.id);
      gAllSelectedProducts.erase(entry.id);

      printf("removing %s from bill\n", entry.short_name.c_str());
    } else {
      printf("updating %s from bill\n", entry.short_name.c_str());
      gBillData[entry.id].Quantity = scanned;
    }
  }
}
float add_less_pos(float billAmt) {
  lk_dispclr();
  lcd::DisplayText(0, 1, "Discount(Round Off)", 0);
  lcd::DisplayText(3, 1, "Press F3 to Less Amt", 0);
  lcd::DisplayText(4, 2, "ENTER to Continue", 0);
  int x = lk_getkey();
  lk_dispclr();

  if (x == klok::pc::KEYS::KEY_F3) {
    int res = 0;
    char addLess[10] = {0};

    lcd::DisplayText(1, 0, "Type Amount to LESS", 0);
    lcd::DisplayText(2, 0, ("Total : " + tostr(billAmt)).c_str(), 0);
    res = lk_getnumeric(4, 0, (unsigned char*)addLess, 10, strlen(addLess));

    float scanned = 0;

    if (sscanf(addLess, "%f", &scanned) == 1 && res > 0) {
      return scanned;
    } else {
      lk_dispclr();
      lcd::DisplayText(4, 0, "Enter correct Amt", 0);
      lk_getkey();
    }
  } else if (x == klok::pc::KEYS::KEY_ENTER) {
    return 0;
  }

  return 0;
}

void display_bill_summary() {
  lk_dispclr();

  gBillAmt = 0;
  gAllSelectedProducts.clear();

  klok::pc::MenuResult res;
  res.wasCancelled = false;
  res.selectedIndex = -1;

  std::vector<BillSummaryDisplayEntry> productDisplayList;
  while (true) {
    productDisplayList.clear();

    gBillAmt = 0;
    for (BillIter_t iter = gBillData.begin(); iter != gBillData.end(); ++iter) {
      klok::pc::Product selected;

      if (klok::pc::Product::FromDatabase(getDatabase(), iter->first.c_str(),
                                          selected) == 0) {
        BillSummaryDisplayEntry newEntry;
        newEntry.id = selected.id;
        newEntry.code = selected.code;
        newEntry.short_name = selected.name;
        newEntry.details = iter->second;
        productDisplayList.push_back(newEntry);
      }

      gBillAmt += iter->second.SalesRate * iter->second.Quantity;
      printf("%s - { %f,%f }\n", iter->first.c_str(), iter->second.SalesRate,
             iter->second.Quantity);
    }

    char totalAmountLabel[30] = {0};
    snprintf(totalAmountLabel, sizeof(totalAmountLabel) - 1,
             "Total Amt : %.02f", gBillAmt);

    klok::pc::display_sub_range_with_title(productDisplayList, totalAmountLabel,
                                           4, res,
                                           &BillSummaryDisplayEntryToString);

    if (!res.wasCancelled) {
      edit_product_quantity(productDisplayList[res.selectedIndex]);
      if (gBillData.count(productDisplayList[res.selectedIndex].id) == 0) {
        productDisplayList.erase(productDisplayList.begin() +
                                 res.selectedIndex);

        if (!productDisplayList.size())
          return;

        gBillAmt = 0;

        for (BillIter_t iter = gBillData.begin(); iter != gBillData.end();
             ++iter) {
          gBillAmt += iter->second.SalesRate * iter->second.Quantity;
          printf("%s - { %f,%f }\n", iter->first.c_str(),
                 iter->second.SalesRate, iter->second.Quantity);
        }
      }

    } else if (klok::pc::KEYS::KEY_F6 == res.lastSpecialKey) {
      const float add_less_amt = add_less_pos(gBillAmt);

      gBillAmt = 0;

      for (BillIter_t iter = gBillData.begin(); iter != gBillData.end();
           ++iter) {
        gBillAmt += iter->second.SalesRate * iter->second.Quantity;
        printf("%s - { %f,%f }\n", iter->first.c_str(), iter->second.SalesRate,
               iter->second.Quantity);
      }

      lk_dispclr();
      lcd::DisplayText(1, 0, ("Gross Amt:   " + tostr(gBillAmt)).c_str(), 0);
      lcd::DisplayText(2, 0, ("Discount:    " + tostr(add_less_amt)).c_str(),
                       0);
      lcd::DisplayText(3, 0, "--------------------", 0);
      lcd::DisplayText(
          4, 0, ("Total:       " + tostr(gBillAmt - add_less_amt)).c_str(), 0);
      lcd::DisplayText(5, 0, "Press Enter to print", 0);

      int x = 0;

      while (true) {
        x = lk_getkey();
        if (x == klok::pc::KEYS::KEY_ENTER || x == klok::pc::KEYS::KEY_CANCEL)
          break;
      }

      if (x == klok::pc::KEYS::KEY_ENTER) {
        klok::pc::PosBillHeader header;

        header.cust_id = "101001";
        header.gross_amt = tostr(gBillAmt);
        header.add_less = tostr(add_less_amt);
        header.net_amt = tostr(gBillAmt - add_less_amt);
        header.date_time = getCurrentTime();
        header.user_id = gUserId;
        header.device_id = gDeviceId;
        header.unique_items = tostr(gBillData.size());
        header.is_deleted = "0";
        header.deleted_at = "";

        if (klok::pc::PosBillHeader::InsertIntoTable(getDatabase(), header) ==
            0) {
          std::string newBillId = "";
          if (klok::pc::PosBillHeader::GetLastBillID(getDatabase(),
                                                     newBillId) != 0) {
            printf("newBillId  is not fetch failed\n");
            return;
          } else if (newBillId == "") {
            printf("newBillId  is not available\n");
            return;
          }

          for (BillIter_t iter = gBillData.begin(); iter != gBillData.end();
               ++iter) {
            klok::pc::PosBillItem item;
            item.bill_id = newBillId;
            item.product_id = tostr(iter->first);
            item.quantity = tostr(iter->second.Quantity);
            item.sales_rate = tostr(iter->second.SalesRate);

            if (klok::pc::PosBillItem::InsertIntoTable(getDatabase(), item) !=
                0) {
              printf("adding item to database failed for bill %s\n",
                     newBillId.c_str());
              return;
            } else {
              printf("adding item  {%s} to database for bill %s\n",
                     item.product_id.c_str(), newBillId.c_str());
            }
          }

          std::string buff, buff1, buff2, buff3, buff4, buff5;

          prn_open();
          if (prn_paperstatus() != 0) {
            lk_dispclr();
            lcd::DisplayText(3, 5, "No Paper !", 1);
            lk_getkey();
            return;
          }

          klok::pos::Configuration c;
          klok::pos::Configuration::ParseFromFile("/mnt/jffs2/POS.cfg", c);

          appendToIfFound(buff, c.getData(), "Company_Title_L1");
          appendToIfFound(buff, c.getData(), "Company_Title_L2");
          appendToIfFound(buff1, c.getData(), "Company_Addr_L1");
          appendToIfFound(buff1, c.getData(), "Company_Addr_L2");
          appendToIfFound(buff1, c.getData(), "Company_Addr_L3");
          appendToIfFound(buff1, c.getData(), "Company_Contact_L1");
          appendToIfFound(buff1, c.getData(), "Company_Contact_L2");
          appendToIfFound(buff1, c.getData(), "Company_Email_L1");
          buff1.append("\n");
          appendToIfFound(buff2, c.getData(), "Bill_name_1");
          appendToIfFound(buff3, c.getData(), "Bill_Line2_1");

          // appendToIfFound(buff2,c.getData(),"Bill_name_2");
          // appendToIfFound(buff2,c.getData(),"Bill_name_3");
          // appendToIfFound(buff2,c.getData(),"Report_name_1");
          // appendToIfFound(buff2,c.getData(),"Report_name_2");
          // appendToIfFound(buff2,c.getData(),"Report_name_3");
          // appendToIfFound(buff2,c.getData(),"Report_name_4");
          // appendToIfFound(buff5,c.getData(),"Print_Footer_L1");
          // appendToIfFound(buff5,c.getData(),"Print_Footer_L2");
          // appendToIfFound(buff5,c.getData(),"Print_Footer_L3");
          // appendToIfFound(buff5,c.getData(),"Print_Footer_L4");
          buff3.append("\n");
          buff3.append(" ---------------------------------------\n");
          buff3.append(" Bill No : ");
          buff3.append("" + gDeviceId + newBillId);
          buff3.append("  Date: " + getCurrentTime());
          buff3.append("\n");
          buff3.append("   ---------------------------------\n");

          buff3.append("NO    Item Name       Qty   Rate     Total\n\n");
          const std::string padding = "                              ";
          for (int i = 0; i != productDisplayList.size(); i++) {
            // buff3.append("     ");
            // buff3.append(tostr(i+1) + "      " +
            //              productDisplayList[i].short_name + "     " +
            //              tostr(productDisplayList[i].details.Quantity) + "  *
            //              " +
            //              tostr(productDisplayList[i].details.SalesRate) + " :
            //              " +
            //              tostr(productDisplayList[i].details.Quantity *
            //                    productDisplayList[i].details.SalesRate));
            // buff3.append("\n");
            const std::string serNoCol = tostr(i + 1) + padding;
            const std::string nameCol =
                " " + productDisplayList[i].short_name + padding;
            const std::string quantityCol =
                padding + tostr(productDisplayList[i].details.Quantity);
            const std::string salesRateCol =
                tostr(productDisplayList[i].details.SalesRate) + padding;
            const std::string totalAmtCol =
                padding + tostr(productDisplayList[i].details.Quantity *
                                productDisplayList[i].details.SalesRate);

            buff3.append(serNoCol.substr(0, 4) + " " + nameCol.substr(0, 14) +
                         quantityCol.substr(quantityCol.size() - 5) + "   " +
                         salesRateCol.substr(0, 6) +
                         totalAmtCol.substr(totalAmtCol.size() - 8) + "\n");
          }

          const std::string grosBillAmtCol = padding + tostr(gBillAmt);
          const std::string cashDiscountCol = padding + tostr(add_less_amt);

          buff3.append("   ---------------------------------\n");
          buff3.append("   Net Amount                  " +
                       grosBillAmtCol.substr(grosBillAmtCol.size() - 10));
          buff3.append("\n");
          buff3.append("   Cash Discount               " +
                       cashDiscountCol.substr(cashDiscountCol.size() - 10));
          buff3.append("\n");
          buff3.append("   ---------------------------------\n");
          buff4.append("TOTAL   :");
          const std::string billTotalCol =
              padding + tostr(gBillAmt - add_less_amt);
          buff4.append(billTotalCol.substr(billTotalCol.size() - 10));
          buff4.append("\n");
          buff5.append("  User  : ");
          buff5.append(gUserName);
          buff5.append("\n");
          buff5.append("   ---------------------------------\n");
          appendToIfFound(buff5, c.getData(), "Print_Footer_L1");
          appendToIfFound(buff5, c.getData(), "Print_Footer_L2");
          appendToIfFound(buff5, c.getData(), "Print_Footer_L3");
          appendToIfFound(buff5, c.getData(), "Print_Footer_L4");

          lk_dispclr();
          lcd::DisplayText(3, 5, "PRINTING BILL", 1);

          int ret;

          ret = printer::WriteText(buff.c_str(), buff.size(), 2);
          returncheck(ret);

          ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
          returncheck(ret);

          ret = printer::WriteText(buff2.c_str(), buff2.size(), 2);
          returncheck(ret);

          ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
          returncheck(ret);

          ret = printer::WriteText(buff4.c_str(), buff4.size(), 2);
          returncheck(ret);

          ret = printer::WriteText(buff5.c_str(), buff5.size(), 1);
          returncheck(ret);

          ret = printer::WriteText("\n\n\n", 3, 1);
          returncheck(ret);
          ret = prn_paper_feed(1);
          prn_close();

          gBillAmt = 0;
          gBillData.clear();
          gAllSelectedProducts.clear();

          if (ret == -3) {
            printf("out of the paper");
          } else {
            return;
          }

        } else {
          printf("failed to InsertIntoTable\n");
        }

        printf("bill saved\n");

        return;

      } else {
        return;
      }
    } else {
      return;
    }
  }
}

void ask_product_quantity(const klok::pc::Product& inProduct) {
  lk_dispclr();
  int res = 0;

  char qty[10] = {0};
  float scanned = 0;

  const std::string stockDisp = "Init Stock : " + inProduct.stock_quantity;

  const float soldStock =
      klok::pc::PosBillItem::GetTotalSold(getDatabase(), inProduct.id.c_str());

  const std::string soldStockDisp = "Sold : " + tostr(soldStock);

  std::string prod_name = "";
  if (inProduct.name.size() >= 12)
    prod_name = inProduct.name.substr(0, 12);
  else
    prod_name = inProduct.name;

  lcd::DisplayText(0, 0, "Enter Quantity", 0);
  lcd::DisplayText(1, 0, (prod_name + " :" + inProduct.sales_rate).c_str(), 0);
  lcd::DisplayText(2, 0, stockDisp.c_str(), 0);
  lcd::DisplayText(3, 0, soldStockDisp.c_str(), 0);
  res = lk_getnumeric(5, 0, (unsigned char*)qty, 10, strlen(qty));
  if (sscanf(qty, "%f", &scanned) == 1 && res > 0) {
    printf("Quantity Entered%f \n", scanned);
    addItemToBill(inProduct, scanned);

    gBillAmt = 0;
    for (BillIter_t iter = gBillData.begin(); iter != gBillData.end(); ++iter) {
      gBillAmt += iter->second.SalesRate * iter->second.Quantity;
      printf("%s - { %f,%f }\n", iter->first.c_str(), iter->second.SalesRate,
             iter->second.Quantity);
    }
  }
}

void BillingByCode() {
  while (true) {
    lk_dispclr();
    char data[256] = {0};
    lcd::DisplayText(2, 0, "Enter Code", 0);
    int res = lk_getnumeric(4, 0, (unsigned char*)data, 10, strlen(data));
    if (res > 0) {
      printf("Code : %s \n", data);

      klok::pc::Product outP;
      if (klok::pc::Product::FromDatabaseWithCode(getDatabase(), data, outP) !=
          0) {
        lk_dispclr();
        lcd::DisplayText(1, 0, "Wrong Code", 0);
        lcd::DisplayText(3, 0, "Try Again - Enter", 0);
        lcd::DisplayText(4, 0, "Cancel - Any other key", 0);
        if (lk_getkey() == klok::pc::KEYS::KEY_ENTER) {
          continue;
        } else {
          return;
        }
      }

      ask_product_quantity(outP);
      break;
    }
  }
}

void Bill_By_Search_Name() {
  while (true) {
    lk_dispclr();
    char data[256] = {0};
    lcd::DisplayText(2, 0, "Enter Search", 0);
    int res = lk_getalpha(3, 0, (unsigned char*)data, 10, strlen(data), 0);
    if (res > 0) {
      printf("Code : %s \n", data);

      std::vector<klok::pc::Product> allProducts;

      if (klok::pc::Product::GetMatchingSearch(getDatabase(), data, allProducts,
                                               500) == 0 &&
          allProducts.size()) {
        klok::pc::MenuResult res;
        res.wasCancelled = false;
        res.selectedIndex = -1;

        char totalAmountLabel[30] = {0};
        snprintf(totalAmountLabel, sizeof(totalAmountLabel) - 1,
                 "Total Amt : %.02f", gBillAmt);

        klok::pc::display_sub_range_with_title(allProducts, totalAmountLabel, 5,
                                               res, &getPosProductDisplayName);

        if (!res.wasCancelled) {
          gProductName = allProducts[res.selectedIndex].name;
          ask_product_quantity(allProducts[res.selectedIndex]);
        } else if (klok::pc::KEYS::KEY_F6 == res.lastSpecialKey) {
          printf("User wants to proceed to billing\n");
          display_bill_summary();
          printf("User finished with billing\n");

        } else {
          break;
        }

      } else {
        lk_dispclr();
        lcd::DisplayText(1, 0, "No match found", 0);

        lcd::DisplayText(3, 0, "Continue - Enter", 0);
        lcd::DisplayText(4, 0, "Cancel - Any Key", 0);

        if (lk_getkey() == klok::pc::KEYS::KEY_ENTER) {
          continue;
        } else {
          return;
        }
      }

      break;
    }
    else return;
  }
}

void Bill_By_Item_List() {
  printf("POS Activity\n");
  lk_dispclr();
  std::vector<klok::pc::Product> allProducts;
  if (klok::pc::Product::GetAllFromDatabase(getDatabase(), allProducts, 500) ==
      0) {
    while (1) {
      // Debug
      // for (int i = 0; i != allProducts.size(); i++) {
      //   printf("ProductId :%s\n", allProducts[i].id.c_str());
      //   printf("ProductName :%s\n", allProducts[i].name.c_str());
      // }

      klok::pc::MenuResult res;
      res.wasCancelled = false;
      res.selectedIndex = -1;

      char totalAmountLabel[30] = {0};
      snprintf(totalAmountLabel, sizeof(totalAmountLabel) - 1,
               "Total Amt : %.02f", gBillAmt);

      klok::pc::display_sub_range_with_title(allProducts, totalAmountLabel, 4,
                                             res, &getPosProductDisplayName);

      if (!res.wasCancelled) {
        gProductName = allProducts[res.selectedIndex].name;
        ask_product_quantity(allProducts[res.selectedIndex]);
      } else if (klok::pc::KEYS::KEY_F6 == res.lastSpecialKey) {
        printf("User wants to proceed to billing\n");
        display_bill_summary();
        printf("User finished with billing\n");

      } else {
        break;
      }
    }

  } else {
    printf("failed to GetAllFromDatabase -> getProductDetails \n");
  }
}

void Pos_Billing_Type_Choice() {
  printf("Billing Choice\n");

  std::vector<std::string> options;
  options.push_back("By Code");
  options.push_back("By Name");
  options.push_back("List All");
  while (1) {
    klok::pc::MenuResult res;
    res.wasCancelled = false;
    res.selectedIndex = -1;

    char totalAmountLabel[30] = {0};
    snprintf(totalAmountLabel, sizeof(totalAmountLabel) - 1,
             "Total Amt : %.02f", gBillAmt);

    klok::pc::display_sub_range_with_title(options, "F6 - Go to bill", 4, res,
                                           &getPosTransactionDatesDisplayName);

    if (!res.wasCancelled) {
      switch (res.selectedIndex) {
        case 0:
          BillingByCode();
          break;
        case 2:
          Bill_By_Item_List();
          break;
        case 1:
          Bill_By_Search_Name();
          break;
      }
    } else if (klok::pc::KEYS::KEY_F6 == res.lastSpecialKey) {
      printf("User wants to proceed to billing\n");
      display_bill_summary();
      printf("User finished with billing\n");

    } else {
      break;
    }
  }
  /*
      MENU_T menu;
      int opt = 0;
      int selItem = 0;
      int acceptKbdEvents = 0;



      while (1) {
        lk_dispclr();

        menu.start = 0;
        menu.maxEntries = 3;
        strcpy(menu.title,"F6 - goto bill");
        strcpy(menu.menu[0], "By Code");
        strcpy(menu.menu[1], "List All");
        strcpy(menu.menu[2], "By Name Search");

        while (1) {
          lk_dispclr();

          lcd::DisplayText(5, 0, "F6 to Continue", 0);
          opt = scroll_menu(&menu, &selItem, acceptKbdEvents);

          switch (opt) {
            case CANCEL:l


            case klok::pc::KEYS::KEY_F6 :
              if(gBillData.size()){
                display_bill_summary();
              }
              else
              {
                lk_dispclr();
                lcd::DisplayText(5, 0, "No Items Selected", 0);
                lk_getkey();
                continue;
              }

                break;


          }
        }
      }
  */
}

void Billing() {
  Pos_Billing_Type_Choice();

  return;

  printf("Billing\n");
  gBillData.clear();
  gBillAmt = 0;
  MENU_T menu;
  int opt = 0;
  int selItem = 0;
  int acceptKbdEvents = 0;

  while (1) {
    lk_dispclr();

    menu.start = 0;
    menu.maxEntries = 2;
    strcpy(menu.menu[0], "Pay Collection");
    strcpy(menu.menu[1], "POS");

    while (1) {
      lk_dispclr();

      opt = scroll_menu(&menu, &selItem, acceptKbdEvents);

      switch (opt) {
        case CANCEL:
          return;

        case ENTER:
          switch (selItem + 1) {
            case 1:
              gMode = Operation::PayCollection;
              PayCollection();
              break;
            case 2:
              gMode = Operation::SpotBilling;
              Pos_Billing_Type_Choice();
              break;
          }
          break;
      }
    }
  }
}

void getDateWiseDetails(std::string date) {
  std::vector<klok::pc::Transaction> allTransactions;
  if (klok::pc::Transaction::GetTransactionsForDate(
          getDatabase(), allTransactions, date.c_str(), 20) == 0) {
    std::string transDate = " Report on " + date;
    prn_open();
    std::string buff, buff1;

    buff.append(transDate);
    buff.append("\n\n");

    int ret;
    ret = printer::WriteText(buff.c_str(), buff.size(), 2);
    if (ret == -3) {
      while (prn_paperstatus() != 0) {
        lk_dispclr();
        lcd::DisplayText(3, 5, "No Paper !", 1);
        int x = lk_getkey();
        if (x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0) {
          if (printer::WriteText(buff.c_str(), buff.size(), 2) != -3)
            break;
        } else if (x == klok::pc::KEYS::KEY_CANCEL) {
          return;
        }
      }
    }

    float totaAmt = 0;
    float parsedNetAmt = 0;

    for (int i = 0; i != allTransactions.size(); i++) {
      printf("Transaction No :%s\n", allTransactions[i].trans_id.c_str());
      printf("Customer Id :%s\n", allTransactions[i].cust_id.c_str());

      std::string buff2;

      buff2.append("    Bill No          ");
      buff2.append(allTransactions[i].trans_id);
      buff2.append("\n");
      buff2.append("    ID               ");
      buff2.append(allTransactions[i].cust_id);
      buff2.append("\n");
      buff2.append("    DATE AND TIME    ");
      buff2.append(allTransactions[i].date_time);
      buff2.append("\n");
      buff2.append("     -------------------------------\n");
      buff2.append("    CASH             ");
      buff2.append(allTransactions[i].net_amt);
      buff2.append("\n");
      buff2.append("     -------------------------------\n");

      sscanf(allTransactions[i].net_amt.c_str(), "%f", &totaAmt);
      parsedNetAmt += totaAmt;

      int ret;

      ret = printer::WriteText(buff2.c_str(), buff2.size(), 1);
      if (ret == -3) {
        while (prn_paperstatus() != 0) {
          lk_dispclr();
          lcd::DisplayText(3, 5, "No Paper !", 1);
          int x = lk_getkey();
          if (x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0) {
            if (printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
              break;
          } else if (x == klok::pc::KEYS::KEY_CANCEL) {
            return;
          }
        }
      }
    }

    char totaAmtString[30] = {0};
    sprintf(totaAmtString, "%0.2f", parsedNetAmt);

    buff1.append("    TOTAL             ");
    buff1.append(totaAmtString);
    buff1.append("\n");
    buff1.append("     -------------------------------\n");

    ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
    if (ret == -3) {
      while (prn_paperstatus() != 0) {
        lk_dispclr();
        lcd::DisplayText(3, 5, "No Paper !", 1);
        int x = lk_getkey();
        if (x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0) {
          if (printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
            break;
        } else if (x == klok::pc::KEYS::KEY_CANCEL) {
          return;
        }
      }
    }

    ret = prn_paper_feed(1);
    prn_close();

  } else {
    printf("failed to GetTransactionsForDate -> getDateWiseDetails \n");
  }
}

void EnteringDate() {
  int res = 0;

  lk_dispclr();
  lcd::DisplayText(1, 0, "Enter Date ", 0);
  lcd::DisplayText(3, 0, "Format : YYYY.MM.DD ", 0);

  char typedBuffer[12] = {0};
  res = lk_getalpha(4, 0, (unsigned char*)typedBuffer, 12, strlen(typedBuffer),
                    0);

  std::string asString = typedBuffer;
  for (int i = 0; i < asString.size(); i++) {
    if (asString[i] == '.') {
      asString[i] = '-';
    }
  }
  if (res > 0) {
    printf("Transactions on %s %d\n", asString.c_str(), res);

    std::vector<klok::pc::Transaction> transObj;
    if (klok::pc::Transaction::GetTransactionsForDate(
            getDatabase(), transObj, asString.c_str(), 20) == 0) {
      getDateWiseDetails(asString);
    }
  }
}

void ListDates() {
  std::vector<std::string> datesUnique;
  if (klok::pc::Transaction::ListUniqueDates(getDatabase(), datesUnique, 20) ==
      0) {
    for (int i = 0; i != datesUnique.size(); i++) {
      printf("Transaction On :%s\n", datesUnique[i].c_str());
    }

    klok::pc::MenuResult res;
    res.wasCancelled = false;
    res.selectedIndex = -1;

    klok::pc::display_sub_range(datesUnique, 5, res,
                                &getPosTransactionDatesDisplayName);

    if (!res.wasCancelled) {
      getDateWiseDetails(datesUnique[res.selectedIndex]);
    }
  } else {
    printf("failed to ListUniqueDates -> ListDates \n");
  }
}

void DailyCollectionReport() {
  printf("DailyCollectionReport Activity\n");

  MENU_T menu;
  int opt = 0;
  int selItem = 0;
  int acceptKbdEvents = 0;

  while (1) {
    lk_dispclr();

    menu.start = 0;
    menu.maxEntries = 2;
    strcpy(menu.menu[0], "By Entering Date");
    strcpy(menu.menu[1], "List Dates");

    while (1) {
      lk_dispclr();

      opt = scroll_menu(&menu, &selItem, acceptKbdEvents);

      switch (opt) {
        case CANCEL:
          return;

        case ENTER:
          switch (selItem + 1) {
            case 1:
              EnteringDate();
              break;

            case 2:
              ListDates();
              break;
          }
          break;
      }
    }
  }
}

void getMonthWiseDetails(std::string month) {
  std::vector<klok::pc::Transaction> allTransactions;
  if (klok::pc::Transaction::GetTransactionsForMonth(
          getDatabase(), allTransactions, month.c_str(), 20) == 0) {
    std::string transMonth = "  Report on " + month;
    prn_open();
    std::string buff, buff1;

    buff.append(transMonth);
    buff.append("\n\n");

    int ret;
    ret = printer::WriteText(buff.c_str(), buff.size(), 2);
    if (ret == -3) {
      while (prn_paperstatus() != 0) {
        lk_dispclr();
        lcd::DisplayText(3, 5, "No Paper !", 1);
        int x = lk_getkey();
        if (x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0) {
          if (printer::WriteText(buff.c_str(), buff.size(), 2) != -3)
            break;
        } else if (x == klok::pc::KEYS::KEY_CANCEL) {
          return;
        }
      }
    }

    float totaAmt = 0;
    float parsedNetAmt = 0;

    for (int i = 0; i != allTransactions.size(); i++) {
      printf("Transaction No :%s\n", allTransactions[i].trans_id.c_str());
      printf("Customer Id :%s\n", allTransactions[i].cust_id.c_str());

      std::string buff2;

      buff2.append("    Bill No          ");
      buff2.append(allTransactions[i].trans_id);
      buff2.append("\n");
      buff2.append("    ID               ");
      buff2.append(allTransactions[i].cust_id);
      buff2.append("\n");
      buff2.append("    DATE AND TIME    ");
      buff2.append(allTransactions[i].date_time);
      buff2.append("\n");
      buff2.append("     -------------------------------\n");
      buff2.append("    CASH             ");
      buff2.append(allTransactions[i].net_amt);
      buff2.append("\n");
      buff2.append("     -------------------------------\n");

      sscanf(allTransactions[i].net_amt.c_str(), "%f", &totaAmt);
      parsedNetAmt += totaAmt;

      int ret;

      ret = printer::WriteText(buff2.c_str(), buff2.size(), 1);
      if (ret == -3) {
        while (prn_paperstatus() != 0) {
          lk_dispclr();
          lcd::DisplayText(3, 5, "No Paper !", 1);
          int x = lk_getkey();
          if (x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0) {
            if (printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
              break;
          } else if (x == klok::pc::KEYS::KEY_CANCEL) {
            return;
          }
        }
      }
    }

    char totaAmtString[30] = {0};
    sprintf(totaAmtString, "%0.2f", parsedNetAmt);

    buff1.append("    TOTAL             ");
    buff1.append(totaAmtString);
    buff1.append("\n");
    buff1.append("     -------------------------------\n");

    ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
    if (ret == -3) {
      while (prn_paperstatus() != 0) {
        lk_dispclr();
        lcd::DisplayText(3, 5, "No Paper !", 1);
        int x = lk_getkey();
        if (x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0) {
          if (printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
            break;
        } else if (x == klok::pc::KEYS::KEY_CANCEL) {
          return;
        }
      }
    }

    ret = prn_paper_feed(1);
    prn_close();
  } else {
    printf("failed to GetTransactionsForMonth -> getMonthWiseDetails \n");
  }
}

void ListMonths() {
  std::vector<std::string> monthsUnique;
  if (klok::pc::Transaction::ListUniqueMonths(getDatabase(), monthsUnique,
                                              20) == 0) {
    for (int i = 0; i != monthsUnique.size(); i++) {
      printf("Transaction No :%s\n", monthsUnique[i].c_str());
    }

    klok::pc::MenuResult res;
    res.wasCancelled = false;
    res.selectedIndex = -1;

    klok::pc::display_sub_range(monthsUnique, 5, res,
                                &getPosTransactionDatesDisplayName);

    if (!res.wasCancelled) {
      getMonthWiseDetails(monthsUnique[res.selectedIndex]);
    }
  } else {
    printf("failed to ListUniqueMonths -> ListMonths \n");
  }
}

void getYearWiseDetails(std::string year) {
  std::vector<klok::pc::Transaction> allTransactions;
  if (klok::pc::Transaction::GetTransactionsForYear(
          getDatabase(), allTransactions, year.c_str(), 20) == 0) {
    std::string transYear = "   Report on " + year;
    prn_open();
    std::string buff, buff1;

    buff.append(transYear);
    buff.append("\n\n");

    int ret;
    ret = printer::WriteText(buff.c_str(), buff.size(), 2);
    if (ret == -3) {
      while (prn_paperstatus() != 0) {
        lk_dispclr();
        lcd::DisplayText(3, 5, "No Paper !", 1);
        int x = lk_getkey();
        if (x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0) {
          if (printer::WriteText(buff.c_str(), buff.size(), 2) != -3)
            break;
        } else if (x == klok::pc::KEYS::KEY_CANCEL) {
          return;
        }
      }
    }

    float totaAmt = 0;
    float parsedNetAmt = 0;

    for (int i = 0; i != allTransactions.size(); i++) {
      printf("Transaction No :%s\n", allTransactions[i].trans_id.c_str());
      printf("Customer Id :%s\n", allTransactions[i].cust_id.c_str());

      std::string buff2;

      buff2.append("    Bill No          ");
      buff2.append(allTransactions[i].trans_id);
      buff2.append("\n");
      buff2.append("    ID               ");
      buff2.append(allTransactions[i].cust_id);
      buff2.append("\n");
      buff2.append("    DATE AND TIME    ");
      buff2.append(allTransactions[i].date_time);
      buff2.append("\n");
      buff2.append("     -------------------------------\n");
      buff2.append("    CASH             ");
      buff2.append(allTransactions[i].net_amt);
      buff2.append("\n");
      buff2.append("     -------------------------------\n");

      sscanf(allTransactions[i].net_amt.c_str(), "%f", &totaAmt);
      parsedNetAmt += totaAmt;

      int ret;

      ret = printer::WriteText(buff2.c_str(), buff2.size(), 1);
      if (ret == -3) {
        while (prn_paperstatus() != 0) {
          lk_dispclr();
          lcd::DisplayText(3, 5, "No Paper !", 1);
          int x = lk_getkey();
          if (x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0) {
            if (printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
              break;
          } else if (x == klok::pc::KEYS::KEY_CANCEL) {
            return;
          }
        }
      }
    }

    char totaAmtString[30] = {0};
    sprintf(totaAmtString, "%0.2f", parsedNetAmt);

    buff1.append("    TOTAL             ");
    buff1.append(totaAmtString);
    buff1.append("\n");
    buff1.append("     -------------------------------\n");

    ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
    if (ret == -3) {
      while (prn_paperstatus() != 0) {
        lk_dispclr();
        lcd::DisplayText(3, 5, "No Paper !", 1);
        int x = lk_getkey();
        if (x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0) {
          if (printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
            break;
        } else if (x == klok::pc::KEYS::KEY_CANCEL) {
          return;
        }
      }
    }

    ret = prn_paper_feed(1);
    prn_close();
  } else {
    printf("failed to GetTransactionsForYear -> getYearWiseDetails \n");
  }
}

void ListYears() {
  std::vector<std::string> yearsUnique;
  if (klok::pc::Transaction::ListUniqueYears(getDatabase(), yearsUnique, 20) ==
      0) {
    for (int i = 0; i != yearsUnique.size(); i++) {
      printf("Transaction No :%s\n", yearsUnique[i].c_str());
    }

    klok::pc::MenuResult res;
    res.wasCancelled = false;
    res.selectedIndex = -1;

    klok::pc::display_sub_range(yearsUnique, 5, res,
                                &getPosTransactionDatesDisplayName);

    if (!res.wasCancelled) {
      getYearWiseDetails(yearsUnique[res.selectedIndex]);
    }
  } else {
    printf("failed to ListUniqueYears -> ListYears \n");
  }
}

void ConsolidatedReport() {
  printf("ConsolidatedReport Activity\n");

  MENU_T menu;
  int opt = 0;
  int selItem = 0;
  int acceptKbdEvents = 0;

  while (1) {
    lk_dispclr();

    menu.start = 0;
    menu.maxEntries = 3;
    strcpy(menu.menu[0], "Daily");
    strcpy(menu.menu[1], "Monthly");
    strcpy(menu.menu[2], "Yearly");

    while (1) {
      lk_dispclr();

      opt = scroll_menu(&menu, &selItem, acceptKbdEvents);

      switch (opt) {
        case CANCEL:
          return;

        case ENTER:
          switch (selItem + 1) {
            case 1:
              ListDates();
              break;

            case 2:
              ListMonths();
              break;

            case 3:
              ListYears();
              break;
          }
          break;
      }
    }
  }
}

void getCustomerWiseDetails(std::string customer) {
  lk_dispclr();

  std::string transCustomer = "  Report of " + customer;
  lcd::DisplayText(1, 0, transCustomer.c_str(), 0);
  lcd::DisplayText(3, 0, "Press ENTER to print , else press CANCEL", 0);

  int x = lk_getkey();

  if (x == klok::pc::KEYS::KEY_ENTER) {
    std::vector<klok::pc::Transaction> allTransactions;
    if (klok::pc::Transaction::GetTransactionsForCustomer(
            getDatabase(), allTransactions, customer.c_str(), 20) == 0) {
      prn_open();
      std::string buff, buff1;

      buff.append(transCustomer);
      buff.append("\n\n");

      int ret;
      ret = printer::WriteText(buff.c_str(), buff.size(), 2);
      if (ret == -3) {
        while (prn_paperstatus() != 0) {
          lk_dispclr();
          lcd::DisplayText(3, 5, "No Paper !", 1);
          int x = lk_getkey();
          if (x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0) {
            if (printer::WriteText(buff.c_str(), buff.size(), 2) != -3)
              break;
          } else if (x == klok::pc::KEYS::KEY_CANCEL) {
            return;
          }
        }
      }

      float totaAmt = 0;
      float parsedNetAmt = 0;

      for (int i = 0; i != allTransactions.size(); i++) {
        printf("Transaction No :%s\n", allTransactions[i].trans_id.c_str());
        printf("Customer Id :%s\n", allTransactions[i].cust_id.c_str());

        std::string buff2;

        buff2.append("    Bill No          ");
        buff2.append(allTransactions[i].trans_id);
        buff2.append("\n");
        buff2.append("    User ID          ");
        buff2.append(allTransactions[i].user_id);
        buff2.append("\n");
        buff2.append("    DATE AND TIME    ");
        buff2.append(allTransactions[i].date_time);
        buff2.append("\n");
        buff2.append("     -------------------------------\n");
        buff2.append("    CASH             ");
        buff2.append(allTransactions[i].net_amt);
        buff2.append("\n");
        buff2.append("     -------------------------------\n");

        sscanf(allTransactions[i].net_amt.c_str(), "%f", &totaAmt);
        parsedNetAmt += totaAmt;

        int ret;

        ret = printer::WriteText(buff2.c_str(), buff2.size(), 1);
        if (ret == -3) {
          while (prn_paperstatus() != 0) {
            lk_dispclr();
            lcd::DisplayText(3, 5, "No Paper !", 1);
            int x = lk_getkey();
            if (x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0) {
              if (printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
                break;
            } else if (x == klok::pc::KEYS::KEY_CANCEL) {
              return;
            }
          }
        }
      }

      char totaAmtString[30] = {0};
      sprintf(totaAmtString, "%0.2f", parsedNetAmt);

      buff1.append("    TOTAL             ");
      buff1.append(totaAmtString);
      buff1.append("\n");
      buff1.append("     -------------------------------\n");

      ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
      if (ret == -3) {
        while (prn_paperstatus() != 0) {
          lk_dispclr();
          lcd::DisplayText(3, 5, "No Paper !", 1);
          int x = lk_getkey();
          if (x == klok::pc::KEYS::KEY_ENTER && prn_paperstatus() == 0) {
            if (printer::WriteText(buff1.c_str(), buff1.size(), 1) != -3)
              break;
          } else if (x == klok::pc::KEYS::KEY_CANCEL) {
            return;
          }
        }
      }

      ret = prn_paper_feed(1);
      prn_close();

    } else {
      printf(
          "failed to GetTransactionsForCustomer -> getCustomerWiseDetails \n");
    }
  } else if (x == klok::pc::KEYS::KEY_CANCEL) {
    printf("pressed cancel while display_customer_details\n");
  }
}

void CustomerWiseReport() {
  std::vector<std::string> customersUnique;
  if (klok::pc::Transaction::ListUniqueCustomers(getDatabase(), customersUnique,
                                                 20) == 0) {
    for (int i = 0; i != customersUnique.size(); i++) {
      printf("Customer Id :%s\n", customersUnique[i].c_str());
    }

    klok::pc::MenuResult res;
    res.wasCancelled = false;
    res.selectedIndex = -1;

    klok::pc::display_sub_range(customersUnique, 5, res,
                                &getPosTransactionDatesDisplayName);

    if (!res.wasCancelled) {
      getCustomerWiseDetails(customersUnique[res.selectedIndex]);
    }
  } else {
    printf("failed to GetAllFromDatabase -> getCustomerDetails \n");
  }
}

void POS_Daily_Report() {
  std::vector<std::string> uniqueDates;

  if (klok::pc::PosBillHeader::ListUniqueDates(getDatabase(), uniqueDates,
                                               100) == 0) {
    klok::pc::MenuResult res;
    res.wasCancelled = false;
    res.selectedIndex = -1;

    klok::pc::display_sub_range(uniqueDates, 5, res,
                                &getPosTransactionDatesDisplayName);

    if (!res.wasCancelled) {
      std::string dateToQuery = uniqueDates[res.selectedIndex];

      float dailyTotal = 0;
      std::vector<klok::pc::PosBillHeader> billsForDate;
      if (klok::pc::PosBillHeader::GetTransactionsForDate(
              getDatabase(), billsForDate, dateToQuery.c_str(), 2000) == 0) {
        std::string buff, buff1, buff2, buff3, buff4, buff5, buffx;
        prn_open();
        if (prn_paperstatus() != 0) {
          lk_dispclr();
          lcd::DisplayText(3, 5, "No Paper !", 1);
          lk_getkey();
          return;
        }

        klok::pos::Configuration c;
        klok::pos::Configuration::ParseFromFile("/mnt/jffs2/POS.cfg", c);

        appendToIfFound(buff, c.getData(), "Company_Title_L1");
        appendToIfFound(buff, c.getData(), "Company_Title_L2");
        appendToIfFound(buff1, c.getData(), "Company_Addr_L1");
        appendToIfFound(buff1, c.getData(), "Company_Addr_L2");
        appendToIfFound(buff1, c.getData(), "Company_Addr_L3");
        appendToIfFound(buff1, c.getData(), "Company_Contact_L1");
        appendToIfFound(buff1, c.getData(), "Company_Contact_L2");
        appendToIfFound(buff1, c.getData(), "Company_Email_L1");
        // appendToIfFound(buff2,c.getData(),"Bill_name_1");
        // appendToIfFound(buff2,c.getData(),"Bill_name_2");
        // appendToIfFound(buff2,c.getData(),"Bill_name_3");
        buff1.append("\n");
        appendToIfFound(buff2, c.getData(), "Report_name_1");
        // appendToIfFound(buff2,c.getData(),"Report_name_2");
        // appendToIfFound(buff2,c.getData(),"Report_name_3");
        // appendToIfFound(buff2,c.getData(),"Report_name_4");
        // appendToIfFound(buff5,c.getData(),"Print_Footer_L1");
        // appendToIfFound(buff5,c.getData(),"Print_Footer_L2");
        // appendToIfFound(buff5,c.getData(),"Print_Footer_L3");
        // appendToIfFound(buff5,c.getData(),"Print_Footer_L4");
        buff3.append("\n");
        buff3.append("     Report Date         :");
        buff3.append(dateToQuery);
        buff3.append("\n");
        buff3.append(" --------------------------------------\n");
        buff3.append(" BillNo    Date & Time      Net Amt   \n");
        buff3.append(" --------------------------------------\n");

        lk_dispclr();
        lcd::DisplayText(3, 5, "PRINTING BILL", 1);
        int ret;
        ret = printer::WriteText(buff.c_str(), buff.size(), 2);
        returncheck(ret);

        ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
        returncheck(ret);

        ret = printer::WriteText(buff2.c_str(), buff2.size(), 2);
        returncheck(ret);

        ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
        returncheck(ret);
        const std::string padding = "                            ";
        for (int i = 0; i < billsForDate.size(); ++i) {
          if (billsForDate[i].is_deleted == "1")
            continue;
          float net_amt_for_bill = 0;

          sscanf(billsForDate[i].net_amt.c_str(), "%f", &net_amt_for_bill);

          dailyTotal += net_amt_for_bill;

          const std::string billIdCol =
              gDeviceId + billsForDate[i].id + padding;
          const std::string dateTimeCol = billsForDate[i].date_time + padding;
          const std::string netAmtCol = padding + billsForDate[i].net_amt;

          buff3 = billIdCol.substr(0, 8) + dateTimeCol.substr(0, 22) +
                  netAmtCol.substr(netAmtCol.size() - 10) + '\n';
          ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
          returncheck(ret);
        }

        buff3 = "     -------------------------------\n";
        ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
        returncheck(ret);

        buffx.append(std::string("TOTAL   :Rs "));
        buffx.append(tostr(dailyTotal));

        ret = printer::WriteText(buffx.c_str(), buffx.size(), 2);
        returncheck(ret);

        ret = printer::WriteText("\n\n\n", 3, 1);
        returncheck(ret);
        ret = prn_paper_feed(1);
        prn_close();

        if (ret == -3) {
          printf("out of the paper");
        } else {
          return;
        }
      }
    }
  }
}

void POS_Total_Report() {
  float dailyTotal = 0;
  std::vector<klok::pc::PosBillHeader> allBills;
  if (klok::pc::PosBillHeader::GetAllNonDeleted(getDatabase(), allBills,
                                                5000) == 0) {
    lk_dispclr();
    lcd::DisplayText(1, 0, "Press Enter to print", 1);
    int x = lk_getkey();

    if (x == klok::pc::KEYS::KEY_ENTER) {
      std::string buff, buff1, buff2, buff3, buffx;
      prn_open();
      if (prn_paperstatus() != 0) {
        lk_dispclr();
        lcd::DisplayText(3, 5, "No Paper !", 1);
        lk_getkey();
        return;
      }

      klok::pos::Configuration c;
      klok::pos::Configuration::ParseFromFile("/mnt/jffs2/POS.cfg", c);

      appendToIfFound(buff, c.getData(), "Company_Title_L1");
      appendToIfFound(buff, c.getData(), "Company_Title_L2");
      appendToIfFound(buff1, c.getData(), "Company_Addr_L1");
      appendToIfFound(buff1, c.getData(), "Company_Addr_L2");
      appendToIfFound(buff1, c.getData(), "Company_Addr_L3");
      appendToIfFound(buff1, c.getData(), "Company_Contact_L1");
      appendToIfFound(buff1, c.getData(), "Company_Contact_L2");
      appendToIfFound(buff1, c.getData(), "Company_Email_L1");
      // appendToIfFound(buff2,c.getData(),"Bill_name_1");
      // appendToIfFound(buff2,c.getData(),"Bill_name_2");
      // appendToIfFound(buff2,c.getData(),"Bill_name_3");
      buff2.append("\n");
      appendToIfFound(buff2, c.getData(), "Report_name_2");
      // appendToIfFound(buff2,c.getData(),"Report_name_2");
      // appendToIfFound(buff2,c.getData(),"Report_name_3");
      // appendToIfFound(buff2,c.getData(),"Report_name_4");
      // appendToIfFound(buff5,c.getData(),"Print_Footer_L1");
      // appendToIfFound(buff5,c.getData(),"Print_Footer_L2");
      // appendToIfFound(buff5,c.getData(),"Print_Footer_L3");
      // appendToIfFound(buff5,c.getData(),"Print_Footer_L4");
      buff3.append("\n");
      buff3.append(" -------------------------------------- \n");
      buff3.append("BillNo   Date & Time          Amount \n");
      buff3.append(" -------------------------------------- \n");
      lk_dispclr();
      lcd::DisplayText(3, 5, "PRINTING BILL", 1);

      int ret;

      ret = printer::WriteText(buff.c_str(), buff.size(), 2);
      returncheck(ret);

      ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
      returncheck(ret);

      ret = printer::WriteText(buff2.c_str(), buff2.size(), 2);
      returncheck(ret);

      ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
      returncheck(ret);

      const std::string padding = "                                   ";
      for (int i = 0; i != allBills.size(); i++) {
        buff3 = "";
        printf("BillId :%s", allBills[i].id.c_str());
        printf(" billDate :%s", allBills[i].date_time.c_str());
        printf(" billAmt :%s\n", allBills[i].net_amt.c_str());

        float net_amt_for_bill = 0;

        sscanf(allBills[i].net_amt.c_str(), "%f", &net_amt_for_bill);

        dailyTotal += net_amt_for_bill;

        const std::string billIdCol = gDeviceId + allBills[i].id + padding;
        const std::string dateTimeCol = allBills[i].date_time + padding;
        const std::string netAmtCol = padding + allBills[i].net_amt;
        buff3.append(billIdCol.substr(0, 7) + dateTimeCol.substr(0, 23) +
                     netAmtCol.substr(netAmtCol.size() - 9) + '\n');
        ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
        returncheck(ret);
      }
      buff3 = "";

      buff3.append(" ------------------------------------\n");

      buffx.append("TOTAL :Rs " + tostr(dailyTotal));

      ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
      returncheck(ret);

      ret = printer::WriteText(buffx.c_str(), buffx.size(), 2);
      returncheck(ret);

      ret = printer::WriteText("\n\n\n", 3, 1);
      returncheck(ret);
      ret = prn_paper_feed(1);
      prn_close();

      if (ret == -3) {
        printf("out of the paper");
      } else {
        return;
      }

    } else if (x == klok::pc::KEYS::KEY_CANCEL) {
      return;
    }

  } else {
    printf("failed to GetAllFromDatabase -> getCustomerDetails \n");
  }
}

struct StockInfo {
  std::string code;
  std::string name;
  float sold;
  std::string stock;
};
void POS_Stock_Report() {
  lk_dispclr();
  lcd::DisplayText(1, 0, "Press Enter to print", 1);
  std::vector<StockInfo> soldSummary;
  typedef std::vector<klok::pc::Product>::iterator Iter_t;

  if (lk_getkey() == klok::pc::KEYS::KEY_ENTER) {
    std::vector<klok::pc::Product> allProducts;

    if (klok::pc::Product::GetAllFromDatabase(getDatabase(), allProducts,
                                              5000) == 0) {
      if (allProducts.size()) {
        for (Iter_t it = allProducts.begin(); it != allProducts.end(); ++it) {
          float totalSold = klok::pc::PosBillItem::GetTotalSold(getDatabase(),
                                                                it->id.c_str());
          if (totalSold != -1 && totalSold > 0) {
            StockInfo entry;
            entry.code = it->code;
            entry.name = it->name;
            entry.stock = it->stock_quantity;
            entry.sold = totalSold;
            soldSummary.push_back(entry);
          }
        }

        if (!soldSummary.size()) {
          lcd::DisplayText(3, 0, "No sales data", 1);
          lk_getkey();
          return;
        }

        std::string buff, buff1, buff2, buff3, buffx;
        prn_open();

        if (prn_paperstatus() != 0) {
          lk_dispclr();
          lcd::DisplayText(3, 5, "No Paper !", 1);
          lk_getkey();
          return;
        }

        klok::pos::Configuration c;
        klok::pos::Configuration::ParseFromFile("/mnt/jffs2/POS.cfg", c);

        appendToIfFound(buff, c.getData(), "Company_Title_L1");
        appendToIfFound(buff, c.getData(), "Company_Title_L2");
        appendToIfFound(buff1, c.getData(), "Company_Addr_L1");
        appendToIfFound(buff1, c.getData(), "Company_Addr_L2");
        appendToIfFound(buff1, c.getData(), "Company_Addr_L3");
        appendToIfFound(buff1, c.getData(), "Company_Contact_L1");
        appendToIfFound(buff1, c.getData(), "Company_Contact_L2");
        appendToIfFound(buff1, c.getData(), "Company_Email_L1");
        // appendToIfFound(buff2,c.getData(),"Bill_name_1");
        // appendToIfFound(buff2,c.getData(),"Bill_name_2");
        // appendToIfFound(buff2,c.getData(),"Bill_name_3");
        // appendToIfFound(buff2, c.getData(), "Report_name_1");
        // appendToIfFound(buff2,c.getData(),"Report_name_2");
        buff2.append("\n");
        appendToIfFound(buff2, c.getData(), "Report_name_3");
        // appendToIfFound(buff2,c.getData(),"Report_name_4");
        // appendToIfFound(buff5,c.getData(),"Print_Footer_L1");
        // appendToIfFound(buff5,c.getData(),"Print_Footer_L2");
        // appendToIfFound(buff5,c.getData(),"Print_Footer_L3");
        // appendToIfFound(buff5,c.getData(),"Print_Footer_L4");
        buff3.append("\n");
        buff3.append("   ---------------------------------\n");
        buff3.append(" Code    Name      Stock      Sold \n");
        buff3.append(" --------------------------------------\n");

        lk_dispclr();
        lcd::DisplayText(3, 5, "PRINTING BILL", 1);
        // Print report here
        int ret;

        ret = printer::WriteText(buff.c_str(), buff.size(), 2);
        returncheck(ret);

        ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
        returncheck(ret);

        ret = printer::WriteText(buff2.c_str(), buff2.size(), 2);
        returncheck(ret);

        ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
        returncheck(ret);

        const std::string padding = "                        ";
        for (int i = 0; i < soldSummary.size(); ++i) {
          const std::string codeCol = soldSummary[i].code + padding;
          const std::string nameCol =
              soldSummary[i].name.substr(0, 9) + padding;
          const std::string stockCol = soldSummary[i].stock + padding;
          const std::string soldkCol = tostr(soldSummary[i].sold) + padding;

          buff3.assign(codeCol.substr(0, 6) + nameCol.substr(0, 14) +
                       stockCol.substr(0, 10) + soldkCol.substr(0, 8) + '\n');
          ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
          returncheck(ret);
        }

        buffx.append("     -------------------------------\n");

        // buffx.append(std::string("    TOTAL          :Rs "));
        // buffx.append(tostr(totalSold));

        ret = printer::WriteText(buffx.c_str(), buffx.size(), 1);
        returncheck(ret);

        ret = printer::WriteText("\n\n\n", 3, 1);
        returncheck(ret);
        ret = prn_paper_feed(1);
        prn_close();

        if (ret == -3) {
          printf("out of the paper");
        } else {
          return;
        }
      }

    } else {
      lcd::DisplayText(3, 0, "Reading database failed", 1);
      lk_getkey();
    }
  };
}

void POS_Deleted_Bill_Report() {
  float dailyTotal = 0;
  typedef std::vector<klok::pc::PosBillHeader>::iterator Iter_t;
  std::vector<klok::pc::PosBillHeader> allBills;
  if (klok::pc::PosBillHeader::GetAllFromDatabase(getDatabase(), allBills,
                                                  5000) == 0) {
    if (allBills.size()) {
      // print bill id , date_time , net_amt , add / less , user id

      std::string buff, buff1, buff2, buff3, buffx;
      prn_open();

      if (prn_paperstatus() != 0) {
        lk_dispclr();
        lcd::DisplayText(3, 5, "No Paper !", 1);
        lk_getkey();
        return;
      }

      lk_dispclr();
      lcd::DisplayText(3, 5, "PRINTING BILL", 1);

      klok::pos::Configuration c;
      klok::pos::Configuration::ParseFromFile("/mnt/jffs2/POS.cfg", c);

      appendToIfFound(buff, c.getData(), "Company_Title_L1");
      appendToIfFound(buff, c.getData(), "Company_Title_L2");
      appendToIfFound(buff1, c.getData(), "Company_Addr_L1");
      appendToIfFound(buff1, c.getData(), "Company_Addr_L2");
      appendToIfFound(buff1, c.getData(), "Company_Addr_L3");
      appendToIfFound(buff1, c.getData(), "Company_Contact_L1");
      appendToIfFound(buff1, c.getData(), "Company_Contact_L2");
      appendToIfFound(buff1, c.getData(), "Company_Email_L1");
      // appendToIfFound(buff2,c.getData(),"Bill_name_1");
      // appendToIfFound(buff2,c.getData(),"Bill_name_2");
      // appendToIfFound(buff2,c.getData(),"Bill_name_3");
      // appendToIfFound(buff2, c.getData(), "Report_name_1");
      // appendToIfFound(buff2,c.getData(),"Report_name_2");
      // appendToIfFound(buff2,c.getData(),"Report_name_3");
      buff2.append("\n");
      appendToIfFound(buff2, c.getData(), "Report_name_4");
      // appendToIfFound(buff5,c.getData(),"Print_Footer_L1");
      // appendToIfFound(buff5,c.getData(),"Print_Footer_L2");
      // appendToIfFound(buff5,c.getData(),"Print_Footer_L3");
      // appendToIfFound(buff5,c.getData(),"Print_Footer_L4");
      buff3.append("\n");
      buff3.append(" -------------------------------------- \n");
      buff3.append("BillNo   Date & Time          Amount \n");
      buff3.append(" -------------------------------------- \n");

      const std::string padding = "                                          ";
      for (int i = 0; i < allBills.size(); ++i) {
        if (allBills[i].is_deleted == "1") {
          const std::string billIdCol = gDeviceId + allBills[i].id + padding;
          const std::string dateTimeCol = allBills[i].date_time + padding;
          const std::string netAmtCol = padding + allBills[i].net_amt;
          buff3.append(billIdCol.substr(0, 7) + dateTimeCol.substr(0, 23) +
                       netAmtCol.substr(netAmtCol.size() - 9) + '\n');

          // printf(" Bill id :%s", allBills[i].id.c_str());
          // printf(" billDate :%s", allBills[i].date_time.c_str());
          // printf(" billAmt :%s", allBills[i].gross_amt.c_str());
          // printf(" billAmt :%s", allBills[i].add_less.c_str());
          // printf(" billAmt :%s", allBills[i].net_amt.c_str());
          // printf(" billAmt :%s\n", allBills[i].user_id.c_str());
        }
      }

      buffx.append(" --------------------------------------\n");

      // buffx.append(std::string("    TOTAL          :Rs "));
      // buffx.append(tostr(totalSold));
      int ret;

      ret = printer::WriteText(buff.c_str(), buff.size(), 2);
      returncheck(ret);

      ret = printer::WriteText(buff1.c_str(), buff1.size(), 1);
      returncheck(ret);

      ret = printer::WriteText(buff2.c_str(), buff2.size(), 2);
      returncheck(ret);

      ret = printer::WriteText(buff3.c_str(), buff3.size(), 1);
      returncheck(ret);

      ret = printer::WriteText(buffx.c_str(), buffx.size(), 1);
      returncheck(ret);

      ret = printer::WriteText("\n\n\n", 3, 1);
      returncheck(ret);
      ret = prn_paper_feed(1);
      prn_close();

      if (ret == -3) {
        printf("out of the paper");
      } else {
        return;
      }

    } else

    {
      lk_dispclr();
      lcd::DisplayText(2, 0, "No Sales", 1);
      lk_getkey();
      return;
    }
  }
}

void Reports() {
  printf("Reports\n");

  MENU_T menu;
  int opt = 0;
  int selItem = 0;
  int acceptKbdEvents = 0;

  while (1) {
    lk_dispclr();

    menu.start = 0;
    menu.maxEntries = 4;
    strcpy(menu.menu[0], "POS Day Report");
    strcpy(menu.menu[1], "POS Total Report");
    strcpy(menu.menu[2], "POS Stock Report");
    strcpy(menu.menu[3], "Cancelled Bill Rpt");
    strcpy(menu.menu[4], "Daily Collection Rpt");
    strcpy(menu.menu[5], "Consolidated Report");
    strcpy(menu.menu[6], "Customer Wise Report");
    while (1) {
      lk_dispclr();

      opt = scroll_menu(&menu, &selItem, acceptKbdEvents);

      switch (opt) {
        case CANCEL:
          return;

        case ENTER:
          switch (selItem + 1) {
            case 1:
              POS_Daily_Report();
              break;

            case 2:
              POS_Total_Report();
              break;

            case 3:
              POS_Stock_Report();
              break;
            case 4:
              POS_Deleted_Bill_Report();
              break;
            case 5:
              ConsolidatedReport();
              break;
            case 6:
              CustomerWiseReport();
              break;
            case 7:
              DailyCollectionReport();
              break;
          }
          break;
      }
    }
  }
}

bool mountUSB(){
    int ret = 0;
  FILE* fp;

  fp = fopen("/etc/mtab", "r");
  char str[100] = "", flag = 0;

  if (fp == NULL)
    fprintf(stderr, "File open Error\n");

  while ((fgets(str, 80, fp)) != NULL) {
    if ((strstr(str, "/mnt/usb")) != NULL)
      flag = 1;
  }

  fclose(fp);

  if(flag == 1) return true;


    ret = system("mount -t vfat /dev/sda1 /mnt/usb");
    if (ret == 256)
      ret = system("mount -t vfat /dev/sdb1 /mnt/usb");
    if (ret == 256)
      ret = system("mount -t vfat /dev/sdc1 /mnt/usb");
    if (ret == 256)
      ret = system("mount -t vfat /dev/sdd1 /mnt/usb");

    return ret == 0;
}

void Export() {
  lk_dispclr();
  lcd::DisplayText(2, 0, "Insert the usb device and press ENTER", 0);
  printf("Export Activity\n");

  int ret = 0;
  FILE* fp;

  fp = fopen("/etc/mtab", "r");
  char str[100] = "", flag = 0;

  if (fp == NULL)
    fprintf(stderr, "File open Error\n");

  while ((fgets(str, 80, fp)) != NULL) {
    if ((strstr(str, "/mnt/usb")) != NULL)
      flag = 1;
  }

  fclose(fp);

  int x = lk_getkey();

  if (x == klok::pc::KEYS::KEY_ENTER && flag == 1) {
    lk_dispclr();
    lcd::DisplayText(2, 2, "Already Mounted", 1);
    lcd::DisplayText(4, 0, "Press any key to copy database to USB", 0);

    lk_getkey();
    ret = system("cp /mnt/jffs2/PayCollect.db /mnt/usb/");

    lk_dispclr();
    lcd::DisplayText(3, 2, "Copying ....", 0);
    lcd::DisplayText(5, 0, "This may take 5-8 seconds", 0);
    sleep(6);

    if (ret == 0) {
      lk_dispclr();
      lcd::DisplayText(3, 2, "Copying Successfull", 0);
      lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
      lk_getkey();

      lk_dispclr();
      lcd::DisplayText(2, 2, "Unmounting disk ..", 0);
      ret = system("umount /mnt/usb");
      lk_dispclr();

      if (ret == 0) {
        lcd::DisplayText(3, 2, "Unmout Successfull", 0);
      } else {
        lcd::DisplayText(3, 2, "Unmounting  Failed....", 0);
        lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
        lk_getkey();
      }
    } else {
      lcd::DisplayText(3, 2, "Copying  Failed....", 0);
      lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
      ret = system("umount /mnt/usb");
      lk_getkey();
    }

    return;

  } else if (x == klok::pc::KEYS::KEY_ENTER && flag == 0) {
    ret = system("mount -t vfat /dev/sda1 /mnt/usb");
    if (ret == 256)
      ret = system("mount -t vfat /dev/sdb1 /mnt/usb");
    if (ret == 256)
      ret = system("mount -t vfat /dev/sdc1 /mnt/usb");
    if (ret == 256)
      ret = system("mount -t vfat /dev/sdd1 /mnt/usb");

    if (ret == 0) {
      lk_dispclr();
      fprintf(stdout, "mass storage mounted\n");
      lcd::DisplayText(2, 2, "MOUNT SUCCESS", 1);
      lcd::DisplayText(4, 0, "Press any key to copy database to USB", 0);

      lk_getkey();
      lk_dispclr();
      lcd::DisplayText(3, 2, "Copying ....", 0);
      lcd::DisplayText(5, 0, "This may take 5-8 seconds", 0);
      ret = system("cp /mnt/jffs2/PayCollect.db /mnt/usb/");

      if (ret == 0) {
        lk_dispclr();
        lcd::DisplayText(3, 2, "Copying Successfull", 0);
        lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
        lk_getkey();

        lk_dispclr();
        lcd::DisplayText(2, 2, "Unmounting disk ..", 0);
        ret = system("umount /mnt/usb");
        lk_dispclr();

        if (ret == 0) {
          lcd::DisplayText(3, 2, "Unmout Successfull", 0);
        } else {
          lcd::DisplayText(3, 2, "Unmounting  Failed....", 0);
          lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
          lk_getkey();
        }
      } else {
        lcd::DisplayText(3, 2, "Copying  Failed....", 0);
        lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
        ret = system("umount /mnt/usb");
        lk_getkey();
      }

      return;
    } else {
      lk_dispclr();
      fprintf(stderr, "Mass storage mounting Failed \n");
      lcd::DisplayText(3, 2, "MOUNT FAILED", 1);
      lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
      lk_getkey();
      return;
    }
  }
}

void Import() {
  lk_dispclr();
  lcd::DisplayText(2, 0, "Insert the usb device and press ENTER", 0);
  printf("Import Activity\n");

  int ret = 0;
  FILE* fp;

  fp = fopen("/etc/mtab", "r");
  char str[100] = "", flag = 0;

  if (fp == NULL)
    fprintf(stderr, "File open Error\n");

  while ((fgets(str, 80, fp)) != NULL) {
    if ((strstr(str, "/mnt/usb")) != NULL)
      flag = 1;
  }

  fclose(fp);

  int x = lk_getkey();

  if (x == klok::pc::KEYS::KEY_ENTER && flag == 1) {
    lk_dispclr();
    lcd::DisplayText(2, 2, "Already Mounted", 1);
    lcd::DisplayText(4, 0, "Press any key to copy database to device", 0);

    lk_getkey();
    lk_dispclr();
    lcd::DisplayText(3, 2, "Copying ....", 0);
    lcd::DisplayText(5, 0, "This may take 5-8 seconds", 0);
    ret = system("cp /mnt/usb/PayCollect.db /mnt/jffs2/");

    if (ret == 0) {
      lk_dispclr();
      closeDatabase();
      lcd::DisplayText(3, 2, "Copying Successfull", 0);
      lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
      lk_getkey();

      lk_dispclr();
      lcd::DisplayText(2, 2, "Unmounting disk ..", 0);
      ret = system("umount /mnt/usb");

      lk_dispclr();
      if (ret == 0) {
        lcd::DisplayText(3, 2, "Unmout Successfull", 0);
      } else {
        lcd::DisplayText(3, 2, "Unmounting  Failed....", 0);
        lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
        lk_getkey();
      }
    } else {
      lcd::DisplayText(3, 2, "Copying  Failed....", 0);
      lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
      ret = system("umount /mnt/usb");
      lk_getkey();
    }
    return;
  } else if (x == klok::pc::KEYS::KEY_ENTER && flag == 0) {
    ret = system("mount -t vfat /dev/sda1 /mnt/usb");
    if (ret == 256)
      ret = system("mount -t vfat /dev/sda2 /mnt/usb");
    if (ret == 256)
      ret = system("mount -t vfat /dev/sdb1 /mnt/usb");
    if (ret == 256)
      ret = system("mount -t vfat /dev/sdc1 /mnt/usb");
    if (ret == 256)
      ret = system("mount -t vfat /dev/sdd1 /mnt/usb");

    if (ret == 0) {
      lk_dispclr();
      fprintf(stdout, "mass storage mounted\n");
      lcd::DisplayText(2, 2, "MOUNT SUCCESS", 1);
      lcd::DisplayText(4, 0, "Press any key to copy database to device", 0);

      lk_getkey();
      lk_dispclr();
      lcd::DisplayText(3, 2, "Copying ....", 0);
      lcd::DisplayText(5, 0, "This may take 5-8 seconds", 0);
      ret = system("cp /mnt/usb/PayCollect.db /mnt/jffs2/");

      if (ret == 0) {
        lk_dispclr();
        lcd::DisplayText(3, 2, "Copying Successfull", 0);
        lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
        lk_getkey();

        lk_dispclr();
        lcd::DisplayText(2, 2, "Unmounting disk ..", 0);
        ret = system("umount /mnt/usb");
        lk_dispclr();

        if (ret == 0) {
          lcd::DisplayText(3, 2, "Unmout Successfull", 0);
        } else {
          lcd::DisplayText(3, 2, "Unmounting  Failed....", 0);
          lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
          lk_getkey();
        }
      } else {
        lcd::DisplayText(3, 2, "Copying  Failed....", 0);
        lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
        ret = system("umount /mnt/usb");
        lk_getkey();
      }

      return;
    } else {
      lk_dispclr();
      fprintf(stderr, "Mass storage mounting Failed \n");
      lcd::DisplayText(3, 2, "MOUNT FAILED", 1);
      lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
      lk_getkey();
      return;
    }
  }
}

void ClearBillTask() {}

void ClearBill() {
  lk_dispclr();
  lcd::DisplayText(1, 0, "Do you want to Clear all BIlls", 0);
  lcd::DisplayText(4, 0, "Press Enter to Clear else Press Cancel to go back",
                   0);

  int x = lk_getkey();

  if (x == klok::pc::KEYS::KEY_ENTER) {
    if (klok::pc::PosBillItem::DeleteAllFromTable(getDatabase()) == 0 &&
        klok::pc::PosBillHeader::DeleteAllFromTable(getDatabase()) == 0) {
      lk_dispclr();
      lcd::DisplayText(3, 1, "Success!", 1);
      lk_getkey();
    } else {
      printf("Failed to detele PosBillItem::DeleteAllFromTable\n");
    }

  } else if (x == klok::pc::KEYS::KEY_CANCEL) {
    return;
  }
}

void DeleteBill() {
  std::vector<klok::pc::PosBillHeader> allBills;
  if (klok::pc::PosBillHeader::GetAllNonDeleted(getDatabase(), allBills,
                                                2000) == 0 &&
      allBills.size()) {
    klok::pc::MenuResult res;
    res.wasCancelled = false;
    res.selectedIndex = -1;

    klok::pc::display_sub_range(allBills, 5, res, &getPosBillDisplayName);

    if (!res.wasCancelled) {
      lk_dispclr();
      lcd::DisplayText(1, 0, "Do you want to Delete this BIll", 0);
      lcd::DisplayText(4, 0, "Press F2 to Delete / Press Enter to return", 0);

      int x = lk_getkey();

      if (x == klok::pc::KEYS::KEY_F2) {
        if (klok::pc::PosBillHeader::MarkBillAsDeleted(
                getDatabase(), allBills[res.selectedIndex].id.c_str(),
                getCurrentTime().c_str()) == 0) {
          printf("Bill Deleted succfully!%s\n",
                 allBills[res.selectedIndex].id.c_str());
        } else {
          printf("Failed to detele PosBillItem::DeleteAllFromTable\n");
        }

      } else if (x == klok::pc::KEYS::KEY_ENTER) {
        return;
      }
    }

  } else {
    printf("failed to ListAllBills \n");
  }
}

void UploadConfig() {
  lk_dispclr();
  lcd::DisplayText(2, 0, "Insert the usb device and press ENTER", 0);
  int x = lk_getkey();
  printf("Import Activity\n");

  int ret = 0;
  FILE* fp;

  fp = fopen("/etc/mtab", "r");
  char str[100] = "", flag = 0;

  if (fp == NULL)
    fprintf(stderr, "File open Error\n");

  while ((fgets(str, 80, fp)) != NULL) {
    if ((strstr(str, "/mnt/usb")) != NULL)
      flag = 1;
  }

  fclose(fp);


  if (x == klok::pc::KEYS::KEY_ENTER && flag == 1) {
    lk_dispclr();
    lcd::DisplayText(2, 2, "Already Mounted", 1);
    lcd::DisplayText(4, 0, "Press any key to copy Configuration to device", 0);

    lk_getkey();
    ret = system("cp /mnt/usb/POS.cfg /mnt/jffs2/");

    lk_dispclr();
    lcd::DisplayText(3, 2, "Copying ....", 0);
    lcd::DisplayText(5, 0, "This may take 5-8 seconds", 0);
    sleep(6);

    if (ret == 0) {
      lk_dispclr();
      lcd::DisplayText(3, 2, "Copying Successfull", 0);
      lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
      lk_getkey();

      lk_dispclr();
      lcd::DisplayText(2, 2, "Unmounting disk ..", 0);
      ret = system("umount /mnt/usb");

      lk_dispclr();
      if (ret == 0) {
        lcd::DisplayText(3, 2, "Unmout Successfull", 0);
      } else {
        lcd::DisplayText(3, 2, "Unmounting  Failed....", 0);
        lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
        lk_getkey();
      }
    } else {
      lcd::DisplayText(3, 2, "Copying Failed....", 0);
      lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
      ret = system("umount /mnt/usb");
      lk_getkey();
    }
    return;
  } else if (x == klok::pc::KEYS::KEY_ENTER && flag == 0) {
    ret = system("mount -t vfat /dev/sda1 /mnt/usb");
    if (ret == 256)
      ret = system("mount -t vfat /dev/sdb1 /mnt/usb");
    if (ret == 256)
      ret = system("mount -t vfat /dev/sdc1 /mnt/usb");
    if (ret == 256)
      ret = system("mount -t vfat /dev/sdd1 /mnt/usb");

    if (ret == 0) {
      lk_dispclr();
      fprintf(stdout, "mass storage mounted\n");
      lcd::DisplayText(2, 2, "MOUNT SUCCESS", 1);
      lcd::DisplayText(4, 0, "Press any key to copy Configuration to device",
                       0);

      lk_getkey();
      ret = system("cp /mnt/usb/POS.cfg /mnt/jffs2/");

      lk_dispclr();
      lcd::DisplayText(3, 2, "Copying ....", 0);
      lcd::DisplayText(5, 0, "This may take 5-8 seconds", 0);
      sleep(6);

      if (ret == 0) {
        lk_dispclr();
        lcd::DisplayText(3, 2, "Copying Successfull", 0);
        lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
        lk_getkey();

        lk_dispclr();
        lcd::DisplayText(2, 2, "Unmounting disk ..", 0);
        ret = system("umount /mnt/usb");
        lk_dispclr();

        if (ret == 0) {
          lcd::DisplayText(3, 2, "Unmout Successfull", 0);
        } else {
          lcd::DisplayText(3, 2, "Unmounting  Failed....", 0);
          lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
          lk_getkey();
        }
      } else {
        lcd::DisplayText(3, 2, "Copying  Failed....", 0);
        lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
        ret = system("umount /mnt/usb");
        lk_getkey();
      }

      return;
    } else {
      lk_dispclr();
      fprintf(stderr, "Mass storage mounting Failed \n");
      lcd::DisplayText(3, 2, "MOUNT FAILED", 1);
      lcd::DisplayText(5, 0, "Press Any Key to Exit", 0);
      lk_getkey();
      return;
    }
  }
}

bool WriteSalesToCSV(const char* name,
                     SQLite::Database& db,
                     std::string& error,
                     std::vector<klok::pc::PosBillHeader>& allBills) {
  /* format

BillNo,GrossAmount,CashDiscount,NetAmount,DateTime,UserId,DeviceId,Code,SalesRate,Quantity,ItemTotal,BillNumber
  */

  try {
    std::ofstream outFile(name);

    if (!outFile) {
      error = "Cannot open";
      return false;
    }

    outFile << "BillNo,GrossAmount,CashDiscount,NetAmount,DateTime,UserId,"
               "DeviceId,Code,SalesRate,Quantity,ItemTotal,BillNumber\n";
    for (size_t i = 0; i < allBills.size(); ++i) {
      std::vector<klok::pc::PosBillItem> items;

      if (klok::pc::PosBillItem::GetAllForBillId(
              getDatabase(), allBills[i].id.c_str(), items, 2000) != 0) {
        error = "Reading bill detail fail";
        return false;
      }

      if (!items.size()) {
        error = "no items for bill : " + gDeviceId + allBills[i].id;
        return false;
      }

      for (size_t itemIndex = 0; itemIndex < items.size(); ++itemIndex) {
        float qty = 0;
        float sr = 0;
        {
          std::istringstream istr(items[itemIndex].quantity);
          istr >> qty;
        }
        {
          std::istringstream istr(items[itemIndex].sales_rate);
          istr >> sr;
        }
        std::stringstream ss;
        ss << (gDeviceId + allBills[i].id) << ',' << allBills[i].gross_amt
           << ',' << allBills[i].add_less << ',' << allBills[i].net_amt << ','
           << allBills[i].date_time << ',' << allBills[i].user_id << ','
           << allBills[i].device_id << ',' << items[itemIndex].product_id << ','
           << items[itemIndex].sales_rate << ',' << items[itemIndex].quantity
           << ',' << (qty * sr) << ',' << (gDeviceId + allBills[i].id) << '\n';

        // std::cout << "Appending : "  << ss.str();
        outFile << ss.str();
      }
    }

    outFile.close();

  } catch (std::exception& e) {
    error = e.what();
    return false;
  }
  return true;
}

void ExportSalesToUSB() {
  lk_dispclr();
  lcd::DisplayText(1, 0, "Export Sales -> USB", 0);
  lcd::DisplayText(2, 0, "F2 to Start", 0);
  lcd::DisplayText(3, 0, "Any other to Abort", 0);
  int key_pressed = lk_getkey();

  if (klok::pc::KEYS::KEY_F2 != key_pressed)
    return;

  system("rm -f /mnt/jffs2/sales.csv");

  std::vector<klok::pc::PosBillHeader> allBills;
  if (klok::pc::PosBillHeader::GetAllNonDeleted(getDatabase(), allBills,
                                                2000) == 0) {
    std::string err;
    if (!WriteSalesToCSV("/mnt/jffs2/sales.csv", getDatabase(), err,
                         allBills)) {
      lk_dispclr();
      lcd::DisplayText(2, 0, "Writing File Failed", 0);
      lcd::DisplayText(4, 0, err.c_str(), 0);
      lk_getkey();
      return;
    }

    while (true) {
      int psRet =
          system("/home/devkermit -s /mnt/jffs2/sales.csv -a sales.csv");

      lk_dispclr();
      if (psRet != 0) {
        lcd::DisplayText(1, 0, "Download failed!", 0);
        lcd::DisplayText(2, 0, ("Status : " + tostr(psRet)).c_str(), 0);
        lcd::DisplayText(4, 0, "Enter - Retry,Other - Abort", 0);
        key_pressed = lk_getkey();

        if (klok::pc::KEYS::KEY_ENTER == key_pressed) {
          continue;
        } else
          return;
      } else {
        lcd::DisplayText(1, 0, "Download Success!", 0);
        lcd::DisplayText(4, 0, "Any Key - OK", 0);
        key_pressed = lk_getkey();
        return;
      }
    }
  } else {
    lk_dispclr();
    lcd::DisplayText(4, 0, "Getting bills failed", 0);
    lk_getkey();
    return;
  }
}

void ExportSalesToDrive() {
  lk_dispclr();
  lcd::DisplayText(1, 0, "Export Sales -> Drive", 0);
  lcd::DisplayText(2, 0, "Enter to Start", 0);
  lcd::DisplayText(3, 0, "Cancel to Abort", 0);
  int key_pressed = lk_getkey();

  if(!mountUSB())
  {
    lk_dispclr();
    lcd::DisplayText(1, 0, "Cannot Mount USB", 0);
    lcd::DisplayText(3, 0, "Any Key - Abort", 0);
    lk_getkey();
    return;
  }


  system("rm -f /mnt/usb/sales.csv");

  std::vector<klok::pc::PosBillHeader> allBills;
  if (klok::pc::PosBillHeader::GetAllNonDeleted(getDatabase(), allBills,
                                                2000) == 0) {
    std::string err;
    if (!WriteSalesToCSV("/mnt/usb/sales.csv", getDatabase(), err,
                         allBills)) {
      system("umount /mnt/usb");
      lk_dispclr();
      lcd::DisplayText(2, 0, "Writing File Failed", 0);
      lcd::DisplayText(4, 0, err.c_str(), 0);
      lk_getkey();
      return;
    }

  } else {
    system("umount /mnt/usb");
    lk_dispclr();
    lcd::DisplayText(4, 0, "Getting bills failed", 0);
    lk_getkey();
    return;
  }
  


lk_dispclr();
lcd::DisplayText(1, 0, "Upload Success!", 0);
lcd::DisplayText(4, 0, "Any Key - OK", 0);
key_pressed = lk_getkey();


}

void ImportStockFromUSB() {
  while (1) {
    lk_dispclr();
    lcd::DisplayText(1, 0, "Import Stock <- USB", 0);
    lcd::DisplayText(2, 0, "F2 to Start", 0);
    lcd::DisplayText(3, 0, "Any key to Abort", 0);
    int key_pressed = lk_getkey();
    if (klok::pc::KEYS::KEY_F2 != key_pressed) {
      return;
    }
    lcd::DisplayText(4, 0, "Waiting...", 0);
    system("rm -f /mnt/jffs2/stock.csv");
    int psRet = system("/home/devkermit -r -a /mnt/jffs2/stock.csv");

    lk_dispclr();
    if (psRet != 0) {
      lcd::DisplayText(1, 0, "Upload failed!", 0);
      lcd::DisplayText(2, 0, ("Status : " + tostr(psRet)).c_str(), 0);
      lcd::DisplayText(4, 0, "Enter - Retry,Other - Abort", 0);
      key_pressed = lk_getkey();

      if (klok::pc::KEYS::KEY_ENTER == key_pressed) {
        continue;
      } else
        return;
    } else {

      {
        std::vector<klok::data::ProductStock> outStock;
        std::string outError = "";
        const int ret = klok::data::ProductStock::fromFile("/mnt/jffs2/stock.csv",outError,outStock);

        if(0 != ret)
        {
          lk_dispclr();
          lcd::DisplayText(1, 0, "File Import failed!", 0);
          lcd::DisplayText(2, 0, ("Status : " + outError).c_str(), 0);
          lcd::DisplayText(4, 0, "Any Key - Abort", 0);
          lk_getkey();
          return;
        }

        klok::pc::PosBillItem::DeleteAllFromTable(getDatabase());
        klok::pc::PosBillHeader::DeleteAllFromTable(getDatabase());
        klok::pc::Product::DeleteAllFromDatabase(getDatabase());

        for (size_t i = 0; i < outStock.size(); ++i)
        {
          klok::pc::Product toInsert;
          toInsert.name = outStock[i].Name;
          toInsert.short_name = outStock[i].ShortName;
          toInsert.code = outStock[i].Code;
          toInsert.sales_rate = outStock[i].SalesRate;
          toInsert.stock_quantity = outStock[i].StockQuantity;

          if(0  != klok::pc::Product::InsertNew(getDatabase(),toInsert)){
            lk_dispclr();
            lcd::DisplayText(1, 0, ("inserting : " + outStock[i].Code ).c_str(), 0);
            lcd::DisplayText(2, 0, "failed", 0);
            lcd::DisplayText(4, 0, "Any Key - Abort", 0);
            lk_getkey();
            return;
          }

        }
        
      }

      lcd::DisplayText(1, 0, "Upload Success!", 0);
      lcd::DisplayText(4, 0, "Any Key - OK", 0);
      key_pressed = lk_getkey();
      return;
    }
  }
}

void ImportStockFromDrive() {
  lk_dispclr();
  lcd::DisplayText(1, 0, "Import stock <- Drive", 0);
  lcd::DisplayText(2, 0, "Enter to Start", 0);
  lcd::DisplayText(3, 0, "Cancel to Abort", 0);
  int key_pressed = lk_getkey();

  if(!mountUSB())
  {
    lk_dispclr();
    lcd::DisplayText(1, 0, "Cannot Mount USB", 0);
    lcd::DisplayText(3, 0, "Any Key - Abort", 0);
    lk_getkey();
    return;
  }

  {
  std::vector<klok::data::ProductStock> outStock;
  std::string outError = "";
  const int ret = klok::data::ProductStock::fromFile("/mnt/usb/stock.csv",outError,outStock);

  if(0 != ret)
  {
    system("umount /mnt/usb");

    lk_dispclr();
    lcd::DisplayText(1, 0, "File Import failed!", 0);
    lcd::DisplayText(2, 0, ("Status : " + outError).c_str(), 0);
    lcd::DisplayText(4, 0, "Any Key - Abort", 0);
    lk_getkey();
    return;
  }

  klok::pc::PosBillItem::DeleteAllFromTable(getDatabase());
  klok::pc::PosBillHeader::DeleteAllFromTable(getDatabase());
  klok::pc::Product::DeleteAllFromDatabase(getDatabase());

  for (size_t i = 0; i < outStock.size(); ++i)
  {
    klok::pc::Product toInsert;
    toInsert.name = outStock[i].Name;
    toInsert.short_name = outStock[i].ShortName;
    toInsert.code = outStock[i].Code;
    toInsert.sales_rate = outStock[i].SalesRate;
    toInsert.stock_quantity = outStock[i].StockQuantity;

    if(0  != klok::pc::Product::InsertNew(getDatabase(),toInsert)){
    system("umount /mnt/usb");
    
      lk_dispclr();
      lcd::DisplayText(1, 0, ("inserting : " + outStock[i].Code ).c_str(), 0);
      lcd::DisplayText(2, 0, "failed", 0);
      lcd::DisplayText(4, 0, "Any Key - Abort", 0);
      lk_getkey();
      return;
    }

  }

  system("umount /mnt/usb");
  lk_dispclr();
  lcd::DisplayText(1, 0, "Import Complete", 0);
  lcd::DisplayText(4, 0, "Any Key - OK", 0);
  lk_getkey();

}

}

void ImportUsersFromUSB() {
  while (1) {
    lk_dispclr();
    lcd::DisplayText(1, 0, "Import Users <- USB", 0);
    lcd::DisplayText(2, 0, "F2 to Start", 0);
    lcd::DisplayText(3, 0, "Cancel to Abort", 0);
    int key_pressed = lk_getkey();
    if (klok::pc::KEYS::KEY_F2 != key_pressed) {
      return;
    }
    lcd::DisplayText(4, 0, "Waiting...", 0);
    system("rm -f /mnt/jffs2/users.csv");
    int psRet = system("/home/devkermit -r -a /mnt/jffs2/users.csv");

    lk_dispclr();
    if (psRet != 0) {
      lcd::DisplayText(1, 0, "Upload failed!", 0);
      lcd::DisplayText(2, 0, ("Status : " + tostr(psRet)).c_str(), 0);
      lcd::DisplayText(4, 0, "Enter - Retry,Other - Abort", 0);
      key_pressed = lk_getkey();

      if (klok::pc::KEYS::KEY_ENTER == key_pressed) {
        continue;
      } else
        return;
    } else {

      {

          std::string outError = "";
          std::vector<klok::data::UserData> outUsers;
          const int retUser = klok::data::UserData::fromFile("/mnt/jffs2/users.csv",outError,outUsers);

          if(0 != retUser)
          {
            lcd::DisplayText(1, 0, "Invalid File!", 0);
            lcd::DisplayText(2, 0, outError.c_str(), 0);
            lcd::DisplayText(4, 0, "Any Key - Abort", 0);
            lk_getkey();
            return;
          }


          klok::pc::User::DeleteAllFromDatabase(getDatabase());


          for (size_t i = 0; i < outUsers.size(); ++i)
          {
            klok::pc::User usr;
            usr.id = outUsers[i].Code;
            usr.name = outUsers[i].Name;
            usr.password = outUsers[i].Password;
            if(0 != klok::pc::User::InsertNew(getDatabase(),usr)){
              lk_dispclr();
              lcd::DisplayText(1, 0, ("inserting : " + outUsers[i].Code ).c_str(), 0);
              lcd::DisplayText(2, 0, "failed", 0);
              lcd::DisplayText(4, 0, "Any Key - Abort", 0);
              lk_getkey();
              return;
            }
          }

      }

      lcd::DisplayText(1, 0, "Upload Success!", 0);
      lcd::DisplayText(3, 0, "Any Key - Reboot", 0);
      key_pressed = lk_getkey();
      closeDatabase();
      system("/sbin/shutdown -r now");
      return;
    }
  }
}

void ImportUsersFromDrive() {
  lk_dispclr();
  lcd::DisplayText(1, 0, "Import Users <- Drive", 0);
  lcd::DisplayText(2, 0, "Enter to Start", 0);
  lcd::DisplayText(3, 0, "Cancel to Abort", 0);
  int key_pressed = lk_getkey();
  if(!mountUSB())
  {
    lk_dispclr();
    lcd::DisplayText(1, 0, "Cannot Mount USB", 0);
    lcd::DisplayText(3, 0, "Any Key - Abort", 0);
    lk_getkey();
    return;
  }

        {

          std::string outError = "";
          std::vector<klok::data::UserData> outUsers;
          const int retUser = klok::data::UserData::fromFile("/mnt/usb/users.csv",outError,outUsers);

          if(0 != retUser)
          {
    system("umount /mnt/usb");

            lcd::DisplayText(1, 0, "Invalid File!", 0);
            lcd::DisplayText(2, 0, outError.c_str(), 0);
            lcd::DisplayText(4, 0, "Any Key - Abort", 0);
            lk_getkey();
            return;
          }


          klok::pc::User::DeleteAllFromDatabase(getDatabase());


          for (size_t i = 0; i < outUsers.size(); ++i)
          {
            klok::pc::User usr;
            usr.id = outUsers[i].Code;
            usr.name = outUsers[i].Name;
            usr.password = outUsers[i].Password;
            if(0 != klok::pc::User::InsertNew(getDatabase(),usr)){
              lk_dispclr();
    system("umount /mnt/usb");
            
              lcd::DisplayText(1, 0, ("inserting : " + outUsers[i].Code ).c_str(), 0);
              lcd::DisplayText(2, 0, "failed", 0);
              lcd::DisplayText(4, 0, "Any Key - Abort", 0);
              lk_getkey();
              return;
            }
          }

      }

      lcd::DisplayText(1, 0, "Upload Success!", 0);
      lcd::DisplayText(3, 0, "Any Key - Reboot", 0);
      key_pressed = lk_getkey();
    system("umount /mnt/usb");

      closeDatabase();
      system("/sbin/shutdown -r now");
      return;
}

void AdminArea() {
  MENU_T menu;
  int opt = 0;
  int selItem = 0;
  int acceptKbdEvents = 0;

  while (1) {
    lk_dispclr();

    strcpy(menu.title,"");
    menu.start = 0;
    menu.maxEntries = 11;
    strcpy(menu.menu[0], "Export DB -> USB");
    strcpy(menu.menu[1], "Import DB <- USB");

    strcpy(menu.menu[2], "Print Config <- USB");

    strcpy(menu.menu[3], "Clear All Bills");
    strcpy(menu.menu[4], "Delete Bill");

    strcpy(menu.menu[5], "Export Sales -> PC");
    strcpy(menu.menu[6], "Export Sales -> USB");

    strcpy(menu.menu[7], "Import Stock <- PC");
    strcpy(menu.menu[8], "Import Stock <- USB");

    strcpy(menu.menu[9], "Import Users <- PC");
    strcpy(menu.menu[10], "Import Users <- USB");
    // strcpy(menu.menu[4],"Logout");
    while (1) {
      lk_dispclr();

      opt = scroll_menu(&menu, &selItem, acceptKbdEvents);

      switch (opt) {
        case CANCEL:
          return;

        case ENTER:

          switch (selItem + 1) {
            case 1:
              Export();
              break;

            case 2:
              Import();
              break;

            case 3:
              UploadConfig();
              break;

            case 4:
              ClearBill();
              break;

            case 5:
              DeleteBill();
              break;

            case 6:
              ExportSalesToUSB();
              break;

            case 7:
              ExportSalesToDrive();
              break;
            case 8:
              ImportStockFromUSB();
              break;
            case 9:
              ImportStockFromDrive();
              break;
            case 10:
              ImportUsersFromUSB();
              break;
            case 11:
              ImportUsersFromDrive();
              break;
            default:
              break;
              // case 5:
              // Login();
              // break;
          }
          break;
      }
    }
  }
}
static const char* const gAdminUserId = "101010";
void Settings() {
  if (gUserId == gAdminUserId) {
    AdminArea();

  } else {
    lk_dispclr();
    lcd::DisplayText(2, 0, "Admin Password Please!", 0);

    int res = 0;
    char pwd[10] = {0};
    res = lk_getpassword((unsigned char*)pwd, 4, 9);
    if (res > 0) {
      pwd[res] = '\0';

      klok::pc::User userObj;
      if (klok::pc::User::FromDatabase(getDatabase(), gAdminUserId, userObj) !=
          0) {
        printf("No admin found %s\n", gAdminUserId);
        lk_dispclr();
        lcd::DisplayText(2, 2, "No Such user ", 1);
        lk_getkey();
        return;
      }

      if (pwd == userObj.password) {
        AdminArea();
      } else {
        lk_dispclr();
        lcd::DisplayText(3, 0, "Sorry Wrong password!", 0);
        lk_getkey();

        return;
      }
    }
  }
  printf("Settings\n");
}
void main_menu(const char* user, const char* pwd) {
  MENU_T menu;
  int opt = 0;
  int selItem = 0;
  int acceptKbdEvents = 0;

  while (1) {
    lk_dispclr();

    menu.start = 0;
    menu.maxEntries = 3;
    strcpy(menu.menu[0], "Billing");
    strcpy(menu.menu[1], "Reports");
    strcpy(menu.menu[2], "Settings");

    while (1) {
      lk_dispclr();

      opt = scroll_menu(&menu, &selItem, acceptKbdEvents);

      switch (opt) {
        case CANCEL:
          break;

        case ENTER:
          switch (selItem + 1) {
            case 1:
              Billing();
              break;

            case 2:
              Reports();
              break;

            case 3:
              Settings();
              break;
          }
          break;
      }
    }
  }
}

void Login() {
  int res = 0;

  while (1) {
    lk_dispclr();
    lcd::DisplayText(2, 0, "Enter Username", 0);

    char user[10] = {0};
    res = lk_getalpha(4, 0, (unsigned char*)user, 9, strlen(user), 0);

    if (res > 0) {
      user[res] = '\0';

      printf("Username is %s %d\n", user, res);

      klok::pc::User userObj;
      if (klok::pc::User::FromDatabase(getDatabase(), user, userObj) != 0) {
        printf("No Such User %s\n", user);
        lk_dispclr();
        lcd::DisplayText(2, 2, "No Such user ", 1);
        lk_getkey();
        continue;
      }

      if (userObj.id == user) {
        while (1) {
          lk_dispclr();

          lcd::DisplayText(2, 0, "Enter Password", 0);

          char pwd[10] = {0};
          res = lk_getpassword((unsigned char*)pwd, 4, 9);

          if (res > 0) {
            pwd[res] = '\0';
            // printf("Password is %s %d\n", pwd, res);

            if (userObj.password == pwd) {
              gUserId = user;
              gUserName = userObj.name;
              gCompanyName = userObj.company_name;
              gCompanyAddress = userObj.company_address;
              main_menu(user, pwd);
              printf("main_menu\n");
            } else {
              goto AGAIN_ASK_USER_DETAILS;
            }
          }
        }
      }
    }

  AGAIN_ASK_USER_DETAILS:
    while (false)
      ;
  }
}

int main(int argc, const char* argv[]) {
  MENU_T menu;
  int opt = 0;
  int selItem = 0;
  int acceptKbdEvents = 0;

  lk_open();
  mscr_open();
  lk_dispclr();
  lk_dispfont(&(X6x8_bits[0]), 6);
  lk_lcdintensity(24);

  SQLite::Database& db = getDatabase();

  lcd::DisplayText(0, 0, "Klok Innovations", 1);
  lcd::DisplayText(4, 0, "  F1   F2   F3   F4", 0);
  lk_dispbutton((unsigned char*)"Home", (unsigned char*)"Up  ",
                (unsigned char*)"Down", (unsigned char*)"End ");
  lk_buzzer(2);

  char autobuf[80] = {0};
  char buff[80] = {0};
  struct tm intim;
  sprintf(autobuf, "%s-%02d%02d%02d%02d%02d%04d.txt", buff, intim.tm_hour,
          intim.tm_min, intim.tm_sec, intim.tm_mday, intim.tm_mon + 1,
          intim.tm_year + 1900);

  while (1) {
    int key1 = lk_getkey();
    if (key1 != 0xff)
      break;
  }

  lk_dispclr();
  strcpy(menu.title, "Login");

  lk_bkl_timeout(20);

  Login();
}
