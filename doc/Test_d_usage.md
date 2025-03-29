<style>
/* Styles généraux */
body {
    font-family: Arial, sans-serif;
    line-height: 1.6;
    max-width: 800px;
    margin: 40px auto;
    padding: 20px;
    background-color: #f4f4f4;
    color: #333;
}

/* Titres */
h1, h2, h3 {
    color: #2c3e50;
    border-bottom: 2px solid #ddd;
    padding-bottom: 5px;
}

/* Tableau */
table {
    width: 100%;
    border-collapse: collapse;
    margin: 20px 0;
}

table, th, td {
    border: 1px solid #ddd;
}

th, td {
    padding: 10px;
    text-align: left;
}

th {
    background-color: #3498db;
    color: white;
}

/* Liens */
a {
    color: #e74c3c;
    text-decoration: none;
}

a:hover {
    text-decoration: underline;
}

/* Code */
code {
    background-color: #eef;
    padding: 2px 5px;
    border-radius: 4px;
}

pre {
    background-color: #222;
    color: #f8f8f2;
    padding: 10px;
    border-radius: 5px;
    overflow-x: auto;
}

/* Citations */
blockquote {
    margin: 10px 0;
    padding: 10px;
    background: #e3f2fd;
    border-left: 5px solid #2196f3;
}

/* Listes */
ul {
    list-style: square;
    margin-left: 20px;
}


</style>
# Tests d'usage du chat Tchatator

## Connexion au serveur

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Lancer le client et tenter de se connecter au serveur | Le client affiche "Connexion établie avec le serveur.(Code 200)" |  |
| Saisir un cle API valide et un mot de passe correct | Le serveur répond "Nom d'utilisateur reçu" et "Mot de passe reçu", puis envoie un token |  |
| Saisir un nom d'utilisateur invalide | Le serveur refuse la connexion |  |

## Options disponibles dans le chat

### 1. Voir tous les messages

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Demander l'affichage de tous les messages | Le serveur envoie la liste complète des messages |  |
| Vérifier la pagination si trop de messages | Les messages sont affichés par lot si nécessaire |  |

### 2. Voir les messages reçus

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Demander uniquement les messages reçus | Le serveur envoie uniquement les messages reçus |  |
| Vérifier si les nouveaux messages sont bien affichés | Les derniers messages reçus apparaissent |  |

### 3. Voir les messages envoyés

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Demander uniquement les messages envoyés | Le serveur envoie uniquement les messages envoyés |  |
| Vérifier si les messages envoyés apparaissent correctement | Tous les messages envoyés sont affichés |  |

### 4. Voir les messages non lus

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Demander uniquement les messages non lus | Le serveur affiche uniquement les messages non lus |  |
| Lire un message non lu et redemander la liste | Le message lu ne devrait plus apparaître comme non lu |  |

### 5. Envoyer un message

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Envoyer un message à un utilisateur | Le serveur demande le message et confirme l'envoi |  |
| Vérifier que le message est bien reçu par le destinataire | Le destinataire voit le message dans sa liste de réception |  |

### 6. Supprimer un message

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Supprimer un message existant | Le message est supprimé et n'apparaît plus dans la liste |  |
| Tenter de supprimer un message inexistant | Le serveur renvoie un message d'erreur |  |

### 7. Modifier un message

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Modifier un message existant | Le message est mis à jour avec le nouveau contenu |  |
| Vérifier si l'ancien message est bien remplacé | L'ancien message ne doit plus apparaître |  |

### 8. Récupérer les messages en JSON

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Demander l'export des messages | Un fichier JSON est généré et disponible en téléchargement |  |

### 9. Rechercher un utilisateur

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Rechercher un utilisateur existant | Les informations de l'utilisateur sont affichées |  |
| Rechercher un utilisateur inexistant | Le serveur renvoie un message d'erreur |  |

### 10. Bloquer un utilisateur

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Bloquer un utilisateur existant | Le serveur confirme "Blocage de l'utilisateur effectué" |  |
| Vérifier si l'utilisateur bloqué ne peut plus envoyer de messages | Les messages de l'utilisateur bloqué ne sont plus reçus |  |

### 11. Débloquer un utilisateur

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Débloquer un utilisateur bloqué | Le serveur confirme "Blocage de l'utilisateur annulé" |  |
| Vérifier si l'utilisateur débloqué peut envoyer des messages | Les messages de l'utilisateur débloqué sont de nouveau reçus |  |

### 12. Bannir un utilisateur

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Bannir un utilisateur | Le serveur confirme "Bannissement de l'utilisateur effectué" |  |
| Vérifier si l'utilisateur banni ne peut plus se connecter | L'utilisateur ne peut plus accéder au chat |  |

### 13. Débannir un utilisateur

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Débannir un utilisateur | Le serveur confirme "Bannissement de l'utilisateur annulé" |  |
| Vérifier si l'utilisateur débanni peut de nouveau se connecter | L'utilisateur peut se reconnecter au chat |  |

### 14. Se déconnecter

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Se déconnecter du serveur | Le serveur ferme la session et affiche "Au revoir" |  |
| Vérifier que les messages ne sont plus accessibles après déconnexion | L'utilisateur doit se reconnecter pour accéder aux messages |  |

### 15. Quitter l'application

| Action | Résultat attendu | Résultat réel |
|--------|-----------------|--------------|
| Fermer le client proprement | Le serveur met fin à la connexion et ferme la session |  |
| Vérifier que le serveur ne plante pas après une déconnexion client | Le serveur continue de fonctionner pour les autres utilisateurs |  |

