set schema 'tchatator';

create view
    msg as
select
    msg_id,
    content,
    sent_at,
    read_age,
    edited_age,
    user_id_sender,
    user_id_recipient
from
    _msg
where
    deleted_age is null;

create view
    msg_ordered as
select
    *
from
    msg
order by
    sent_at desc,
    msg_id desc; -- secondary sort to solve conflicts for messages sent on the same second: last inserted first

create view
    "user" as
select
    user_id,
    api_key,
    password_hash,
    case
        when a.user_id is not null then 0 -- admin
        when m.user_id is not null then 2 -- member
        when p.user_id is not null then 4 -- pro
    end as role,
    m.user_name member_user_name,
    p.business_name pro_business_name
from
    _user
    left join _admin a using (user_id)
    left join _member m using (user_id)
    left join _pro p using (user_id);

create view
    pro as
select
    *
from
    _pro
    join _user using (user_id);

create view
    member as
select
    *
from
    _member
    join _user using (user_id);

create view
    "admin" as
select
    *
from
    _admin
    join _user using (user_id);