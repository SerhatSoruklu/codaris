# CODARIS Databases & Data Platforms Research Pack

Generated: 23 September 2026

## Scope

This pack contains **643 database and developer-data entries** across A-Z.

It intentionally includes multiple layers that developers commonly evaluate:
- relational/SQL databases
- document databases
- key-value and in-memory stores
- wide-column systems
- graph and RDF stores
- time-series databases
- search/indexing systems
- vector databases and vector-capable databases
- analytical warehouses/lakehouses
- embedded/mobile/edge databases
- object/XML/multivalue/specialty databases
- managed cloud database services
- developer data platforms/BaaS products

The `product_type`, `primary_model`, and `deployment` fields exist so CODARIS does not incorrectly describe every product as the same kind of database.

## Current research snapshots

### DB-Engines, September 2026
DB-Engines lists **438 DBMS products** in its complete September 2026 ranking.
This pack includes the current top 20 and selected model-category counts/leaders.

Important: the DB-Engines score is a **popularity index**, not developer market share or deployment share.

### Stack Overflow Developer Survey 2025
The database question had **26,083 respondents**.
Verified all-respondent usage values captured in the pack include:
- PostgreSQL 55.6%
- MySQL 40.5%
- SQLite 37.5%
- Microsoft SQL Server 30.1%
- Redis 28.0%
- MongoDB 24.0%
- MariaDB 22.5%
- Elasticsearch 16.7%
- Oracle 10.6%
- DynamoDB 9.8%
- BigQuery 6.5%
- Supabase 6.0%
- Cloud Firestore 5.7%
- H2 5.0%
- Firebase Realtime Database 5.0%
- Microsoft Access 4.8%

The question is multi-select, so percentages do not sum to 100%.

## Important taxonomy examples

- **MongoDB** = database engine / DBMS.
- **MongoDB Atlas** = managed MongoDB database platform.
- **Firebase** = developer platform.
- **Cloud Firestore** = Firebase/Google Cloud document database.
- **Firebase Realtime Database** = Firebase realtime JSON database.
- **Supabase** = developer platform whose foundation is a full PostgreSQL database.
- **Neon** = serverless PostgreSQL platform.
- **PlanetScale** = managed relational platform with PostgreSQL, Vitess and Neki offerings.
- **PocketBase** = backend built around embedded SQLite.
- **Convex** = reactive database plus backend platform.
- **Appwrite** = backend platform with multiple database products and managed native PostgreSQL/MySQL.

## Demographic and geography integrity

No per-database male/female percentages or country percentages are fabricated.
Overall Stack Overflow respondent geography is included only as survey-sample context.
Per-database country or demographic claims should be added only from direct cross-tab data with sample sizes.

## Files

- `CODARIS_databases_research.xlsx`
- `codaris_databases_catalog.csv`
- `codaris_databases_catalog.json`
- `codaris_databases_usage_2025.csv`
- `codaris_databases_dbengines_2026_09.csv`
- `codaris_databases_sources.csv`

## Recommended CODARIS database page fields

1. Name
2. Product type
3. Primary data model
4. Secondary models
5. Deployment model
6. Provider/project
7. Stack Overflow usage where available
8. DB-Engines rank/score where available
9. Source and snapshot date
10. Clear tooltip explaining exactly what each popularity metric measures

Never combine Stack Overflow usage and DB-Engines scores into one synthetic "market share" number.
