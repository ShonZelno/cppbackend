#!/usr/bin/env python3
import os
import uuid
import random
import psycopg2
import psycopg2.extras
from psycopg2.extensions import ISOLATION_LEVEL_AUTOCOMMIT

# Установите те же переменные окружения, что и в тестах
os.environ['POSTGRES_USER'] = 'postgres'
os.environ['POSTGRES_PASSWORD'] = 'Mys3Cr3t'
os.environ['POSTGRES_HOST'] = 'localhost'
os.environ['POSTGRES_PORT'] = '30432'

def get_connection(db_name=None):
    return psycopg2.connect(
        user=os.environ['POSTGRES_USER'],
        password=os.environ['POSTGRES_PASSWORD'],
        host=os.environ['POSTGRES_HOST'],
        port=os.environ['POSTGRES_PORT'],
        dbname=db_name if db_name else 'postgres',
        cursor_factory=psycopg2.extras.DictCursor,
    )

def create_full_db():
    print("Создаю базу full_db...")
    
    # Создаем базу
    conn = get_connection(None)
    conn.set_isolation_level(ISOLATION_LEVEL_AUTOCOMMIT)
    with conn.cursor() as cur:
        cur.execute('DROP DATABASE IF EXISTS full_db;')
        cur.execute('CREATE DATABASE full_db;')
    conn.close()

    # Создаем таблицы и наполняем данными
    with get_connection('full_db') as conn:
        with conn.cursor() as cur:
            # Создаем таблицы
            cur.execute("""
                CREATE TABLE IF NOT EXISTS authors (
                    id UUID PRIMARY KEY,
                    name varchar(100) NOT NULL UNIQUE
                );
            """)
            
            cur.execute("""
                CREATE TABLE IF NOT EXISTS books (
                    id UUID PRIMARY KEY,
                    title VARCHAR(100) NOT NULL,
                    publication_year INT,
                    author_id UUID,
                    CONSTRAINT fk_authors
                        FOREIGN KEY(author_id)
                        REFERENCES authors(id)
                );
            """)
            
            cur.execute("""
                CREATE TABLE IF NOT EXISTS book_tags (
                    book_id UUID,
                    tag varchar(30) NOT NULL,
                    CONSTRAINT fk_books
                        FOREIGN KEY(book_id)
                        REFERENCES books(id)
                );
            """)

            # Добавляем авторов
            author_ids = []
            insert_author = 'INSERT INTO authors (id, name) VALUES (%s, %s);'
            for i in range(5):
                _uuid = uuid.uuid4().hex
                author_ids.append(_uuid)
                cur.execute(insert_author, (_uuid, f'author{i}'))
                print(f"Добавлен автор: author{i}")

            # Добавляем книги и теги
            insert_book = 'INSERT INTO books (id, title, author_id, publication_year) VALUES (%s, %s, %s, %s);'
            insert_tag = 'INSERT INTO book_tags (book_id, tag) VALUES (%s, %s);'
            
            for i in range(20):
                book_id = uuid.uuid4().hex
                author_id = random.choice(author_ids)
                year = 1000 + i
                cur.execute(insert_book, (book_id, f'Title{i}', author_id, year))
                print(f"Добавлена книга: Title{i}, {year}")
                
                # Добавляем теги (0-4 штук)
                for j in range(random.randint(0, 4)):
                    cur.execute(insert_tag, (book_id, f'tag{j}'))
                    print(f"  Добавлен тег: tag{j}")

            conn.commit()
    
    print("База full_db успешно создана!")
    print("\nПроверьте данные:")
    print("Авторы: author0, author1, author2, author3, author4")
    print("Книги: Title0-Title19, года 1000-1019")
    print("Теги: tag0-tag3 (0-4 на книгу)")

if __name__ == '__main__':
    create_full_db()