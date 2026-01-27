# Plan 9 Auth Server Setup

## Overview
The Plan 9 authentication server (authsrv) manages user credentials and authentication for the 9P protocol.

## Basic Setup on 9front

### 1. Start the auth server
```rc
# Run on the machine that will be your auth server
auth/authsrv
```

### 2. Initialize the auth database
```rc
# Create the keyfs file system (first time only)
auth/keyfs
```

### 3. Add users
```rc
# Add a new user to the auth database
auth/changeuser glenda
# You'll be prompted for:
# - Password
# - Expiration date (leave blank for never)
# - Post id (can be same as user)
# - Secret (for challenge/response)
```

### 4. Configure the auth server address
Edit `/lib/ndb/local` to add:
```
auth=authserver.local authdom=9grid
```

### 5. Start factotum with auth server
```rc
# On client machines
auth/factotum -a tcp!authserver.local!567
```

## For u9fs with p9any authentication

### 1. Create /etc/u9fs.key on Unix host
```bash
echo "mysecret" > /etc/u9fs.key      # shared secret
echo "glenda" >> /etc/u9fs.key       # user
echo "9grid" >> /etc/u9fs.key        # auth domain
chmod 600 /etc/u9fs.key
```

### 2. Add key to factotum on 9front
```rc
echo 'key proto=p9sk1 dom=9grid user=glenda !password=mysecret' >/mnt/factotum/ctl
```

### 3. Start u9fs with p9any
```bash
u9fs -a p9any -A /etc/u9fs.key /path/to/share
```

## Simple Local Setup (No Auth Server)

For local VM interop without a full auth server:

### Option 1: Trust-based (rhosts)
```bash
# Add to /etc/hosts.equiv on Unix
echo "localhost" >> /etc/hosts.equiv
# Run u9fs
u9fs -a rhosts /path/to/share
```

### Option 2: Fixed user (simple but insecure)
```bash
# Runs as specific Unix user
u9fs -n -a none -u username /path/to/share
```

## Testing Authentication

```rc
# From 9front, mount with auth
srv tcp!unix-host!564 shareauth
mount -c /srv/shareauth /n/share

# Check who you're authenticated as
ns | grep share
```

## Common Issues

1. **"authentication failed"** - Check factotum has the right key
2. **"permission denied"** - u9fs might be defaulting to read-only
3. **"can't write"** - With `-a none`, u9fs often restricts writes

## Security Notes

- `auth/authsrv` should run on a trusted machine
- Keep /etc/u9fs.key protected (mode 600)
- Use p9any for production, none/rhosts only for testing
- The auth server holds all user secrets - protect it well!