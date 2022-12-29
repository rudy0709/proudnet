CREATE TABLE [dbo].[Friend] (
    [UUID]      UNIQUEIDENTIFIER NOT NULL,
    [OwnerUUID] UNIQUEIDENTIFIER NOT NULL,
    [RootUUID]  UNIQUEIDENTIFIER NOT NULL,    
    [charLevel] INT NOT NULL, 
    [Name] VARCHAR(50) NOT NULL, 
	[Comment]   NVARCHAR (50)    NULL,
    CONSTRAINT [PK_Friend] PRIMARY KEY CLUSTERED ([UUID] ASC)
);


GO
CREATE NONCLUSTERED INDEX [IX_Friend_OwnerUUID]
    ON [dbo].[Friend]([OwnerUUID] ASC);


GO
CREATE NONCLUSTERED INDEX [IX_Friend_RootUUID]
    ON [dbo].[Friend]([RootUUID] ASC);

