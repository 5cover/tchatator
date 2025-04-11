set schema 'tchatator';

create view
    msg as
select
    msg_id,
    content,
    sent_at,
    read_age,
    edited_age,
    id_compte_sender,
    id_compte_recipient
from
    _msg;

create view
    inbox as
select
    *
from
    msg
order by
    sent_at;

create view
    user as
select
    *
from _user;