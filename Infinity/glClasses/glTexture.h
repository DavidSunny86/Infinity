/**
 * \ingroup glClasses
 * ���� glTexture.
 * ��� ���� � ������� ��� ������������� �������
 * �� ������ �� ��������������: ���, ����������,
 * ���� ����������� �������������, � ����� ����
 * ���������� ��������� mipmap ���� �� ��������� 
 * ������������.
**/
class Texture
{
	public:
		/**
		 * �����������.
		 * \param target ��� ��������, �� �� ��������� � OpenGL
		 * \param internalFormat ���� ������ ����������� ������������� ����������
		 * \param minFilter ���� ������, �� ��������������� ��� ��������
		 * \param magFilter ���� ������, �� ��������������� ��� ��������
		 * \param forceMipmap �����, �� ���������� ��������� ��� ����������
		**/
		Texture(GLenum target, GLint internalFmt, GLint minFilter, GLint magFilter, bool forceMipmap=true)
		{
			internalFormat_ = internalFmt;
			target_ = target;
			glGenTextures(1, &mHandle);
			glBindTexture(target, mHandle);
			glTexParameteri(target, GL_GENERATE_MIPMAP, forceMipmap);
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilter);
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magFilter);
		}
		/**
		 * ����������
		**/
		~Texture(void){glDeleteTextures(1, &mHandle);}
		
	public:
		/**
		 * ��� ��������.
		**/
		GLenum	target_;
		/**
		 * ���������� ��������.
		**/
		GLuint	mHandle;
		/**
		 * �������� ������ ��������.
		**/
		GLint	internalFormat_;
};

/**
 * \ingroup glClasses
 * ���� ��������� ��������.
 * ̳����� ������ ��� ������ � ��� ����� �������:
 * ������������ ��������� ���������� ���������, �
 * ����� ���������� ��������.
**/
class Texture2D: public Texture
{
	public:
		/**
		 * �����������.
		 * \param internalFormat ���� ������ ����������� ������������� ����������
		 * \param minFilter ���� ������, �� ��������������� ��� ��������
		 * \param magFilter ���� ������, �� ��������������� ��� ��������
		 * \param forceMipmap �����, �� ���������� ��������� ��� ����������
		**/
		Texture2D(GLint internalFmt, GLint minFilter, GLint magFilter, bool forceMipmap=true):
		  Texture(GL_TEXTURE_2D, internalFmt, minFilter, magFilter, forceMipmap){}

		/**
		 * ���� ����� ���������� ���������� ��������� �� ��� [0..1]
		 * \param wrapS ���������� �� �� S(�������������)
		 * \param wrapT ���������� �� �� T(�����������)
		**/
		void setWrapMode(GLint wrapS, GLint wrapT)
		{
			glBindTexture(target_, mHandle);
			glTexParameteri(target_, GL_TEXTURE_WRAP_S, wrapS);
			glTexParameteri(target_, GL_TEXTURE_WRAP_T, wrapT);
		}

		/**
		 * ���� ���������� ��������
		 * \param width ������ ����������
		 * \param height ������ ����������
		 * \param format ������ ���������� - ��� �� ������� ������
		 * \param type ��� �������� ������
		 * \param pixels �������� �� ���
		**/
		void setImage2D(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)//maybe this is not usefullas
		{
			glBindTexture(target_, mHandle);
			glTexImage2D(target_, 0, internalFormat_, width, height, 0, format, type, pixels);
		}
};