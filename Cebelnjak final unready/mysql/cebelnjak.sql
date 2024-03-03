drop database cebelnjak;

select * from panj;
select * from video;

-- ---------------------------------------------------------------------------------

create database cebelnjak;

use cebelnjak;

create table panj(
	id_zapisa datetime not null primary key,
    temp1 int,
    temp2 int,
    svetlost bool,
    dez bool,
    vlaga int,
    teza int,
    zvok_aktivnost int
);

create table video(
	id_videa datetime not null,
    num_datoteke int not null,
    video_path varchar(255),
    primary key (id_videa, num_datoteke)
);


-- create 2 users (full control and read only)
create user "frontend"@"localhost" identified by "";
grant select on cebelnjak.* to  "frontend"@"loclhost";

-- ---------------------------------------------------------------------

delete from panj where date(id_zapisa) <= '2023-05-10';

insert into panj values("2023-05-09", 10, 10, 0, 1, 10, 10, 100);
insert into panj values("2023-04-26", 20, 20, 0, 1, 20, 20, 200);
insert into panj values("2023-01-10", 20, 20, 0, 1, 20, 20, 200);



insert into video values("2023-01-10 23:00:00", 2, "2023-01-10 23-2.avi");



