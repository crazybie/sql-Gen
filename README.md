# sql-Gen
generate sql through cpp expressions.

let you write the following code in pure cpp:

sql=Select()
    .select(userTable.name,classTable.name)
    .from(userTable.join(classTable, classTable.age==userTable.age))
    .where(userTable.age==18)
    .orderBy(userTable.name, OrderDesc);
exe(sql);