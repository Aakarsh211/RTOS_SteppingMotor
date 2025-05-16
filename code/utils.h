/*
 * utils.h
 *
 *  Created on: Jan 17, 2025
 *      Author: Antonio Andara Lara
 */

 #ifndef SRC_UTILS_H_
 #define SRC_UTILS_H_
 
 #define MAX_USERS 3
 #define MAX_LEN         32
 #define HASH_LENGTH     32
 #define HASH_STR_SIZE   ((2 * HASH_LENGTH) + 1)
 
 /* Structure for login data from the user */
 typedef struct {
     char username[MAX_LEN];
     char password[MAX_LEN];
 } LoginData;
 
 /* Structure for a registered user (only the hash is stored) */
 typedef struct {
     char hashString[HASH_STR_SIZE];
 } RegisteredUser;
 
 /* User array */
 RegisteredUser registeredUsers[MAX_USERS] = {
     { "1FBB0D5D6DE2941C19A830C530016E08D63AF89290D2E5F7E6B70DD2EC559DF4" },
     { "C09020DD0097DEF0C6AF392626A1935DE9202260241B701D585E7D99FB4EF56D" },
 };
 
 int registeredUserCount = 2;
 
 bool loggedIn = false;
 
 TickType_t xPollPeriod = 100U;
 
 #endif /* SRC_UTILS_H_ */
 