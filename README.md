# 🌍 ClearData – Base de Données Qualité de l’Air en France

Projet académique de base de données relationnelle, développé pour simuler un outil national de centralisation et d’analyse des données environnementales liées à la qualité de l’air. Le projet a été réalisé en réponse à un cahier des charges fourni par un ministère fictif dans un contexte professionnel simulé.

---

## 📌 Objectif

Créer une base de données permettant :
- La gestion des agences de mesure de la qualité de l’air
- Le suivi des capteurs répartis sur le territoire
- Le stockage des mesures mensuelles de concentration de gaz (en ppm)
- La rédaction et la consultation de rapports environnementaux
- L’extraction de synthèses précises via des requêtes SQL avancées

---

## 🧱 Structure du projet

### 📁 `docs/`
- Cahier des charges analysé
- Dictionnaire de données
- MCD (Modèle Conceptuel de Données)
- MLD (Modèle Logique de Données)
- MPD (Modèle Physique de Données)

### 📁 `sql/`
- `create_database.sql` : Script de création des tables et contraintes
- `insert_data.sql` : Insertion de données réalistes (capteurs, employés, mesures…)
- `queries.sql` : Requêtes demandées (1 à 12)
- `users.sql` : Création des comptes `admin` et `user` avec les droits respectifs

### 📁 `data/`
- `exemples_donnees.xls` : Données brutes fournies
- Données simulées (≥ 200 mesures, 20 employés, 10 rapports)
