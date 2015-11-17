# sql-Gen
generate sql through normal cpp expressions at runtime.

Allow you to write the following code in pure cpp:

string sql=Select()
    .select(userTable.name,classTable.name)
    .from(userTable.join(classTable, classTable.age==userTable.age))
    .where(userTable.age==18)
    .orderBy(userTable.name, OrderDesc);

// send the string to your db driver.