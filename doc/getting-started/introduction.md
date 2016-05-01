1.1 Introduction
================

EventQL is a distributed, analytical database. It allows you to store massive amounts
of structured data and explore it using SQL and other programmatic query facilities.

If you want to take the deep dive, start by reading [the "Concepts" page](../concepts/).

##### Distributed Queries &amp; Storage

Table storage and query execution is distributed across a cluster of physical server,
allowing you to store, process and query massive tables with trillions of records.
Sharding and replication is fully automatic and transparent. Queries are automatically
parallelized and distributed across servers.

##### Column-oriented

EventQL is a column-oriented database. This means that when executing a query, it
doesn't have to read the full row from disk (like row-oriented databases) but
only those columns which are actually required by the query. This feature results
in a dramatic speed-up for most queries but is otherwise transparent to the user.

##### Nested Records

The core unit of data storage in EventQL are tables and rows. Tables have a strict
schema and are strictly typed. Tables in EventQL may have "nested" schema that allow
you to store complex data structures (like a JSON object) in a single field.

##### SQL and JavaScript Queries

In addition to the full SQL query language, EventQL can execute JavaScript queries
and data processing pipelines allowing you to build the most complex data driven
applications.

##### Data Connectors

##### Query Tools & Report Editor

EventQL features a web UI that allows you to create and share complex reports
and dashboards using SQL, JavaScript and built-in graphical analytics and
query tools.

##### Plugins