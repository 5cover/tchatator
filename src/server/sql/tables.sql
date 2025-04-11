set schema 'tchatator';

-- CLASSES

create table _user (
    user_id serial primary key,
    api_key uuid not null,
    password_hash varchar(255) -- null if no password
);

create table _member (
    user_id serial primary key references _user on delete cascade;
    user_name varchar not null;
    full_block_expires_at timestamp;
)

create table _professionnal (
    user_id serial primary key references _user on delete cascade;
    business_name varchar not null;
)

create table _administrator (
    user_id serial primary key references _user on delete cascade;
)

-- todo: use _user instead of pact

create table _msg (
    msg_id serial primary key,
    content varchar not null,
    sent_at timestamp not null default localtimestamp,

    -- ages: sent_at + *_age = *_at
    -- null if never occured
    -- not using intervals because they are hard to deal with and we only need a number of seconds
    read_age int, -- read by recipient
    edited_age int,
    deleted_age int,
    
    constraint deleted_gt_read check (deleted_age > read_age),
    constraint deleted_gt_modified check (deleted_age > edited_age),

    -- null fk for root (0)
    id_compte_sender int
        constraint message_fk_compte_sender references _user on delete cascade,
    id_compte_recipient int
        constraint message_fk_compte_recipient references _user on delete cascade,

    constraint sender_ne_recipient check (id_compte_sender <> id_compte_recipient)
);

-- ASSOCIATIONS

create table _single_block (
    id_member int not null
        constraint single_block_fk_member references _member on delete cascade,
    id_professionnal int not null
        constraint single_block_fk_professionnal references _professionnal on delete cascade,
    constraint single_block_pk primary key (id_member, id_professionnal),

    expires_at timestamp not null default 'infinity'
);
